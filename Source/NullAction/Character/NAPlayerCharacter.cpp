#include "Character/NAPlayerCharacter.h"

#include "NullAction.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Interaction/NAInteractable.h"
#include "Perception/AISense_Hearing.h"
#include "Survival/NASurvivalComponent.h"
#include "Inventory/NAInventoryComponent.h"
#include "Abilities/NAAbilityComponent.h"
#include "Combat/NAHealthComponent.h"
#include "Weapons/NAWeapon.h"
#include "Weapons/NAWeaponTypes.h"
#include "World/NAWaterVolume.h"
#include "World/NAWorldEnvironment.h"
#include "Core/NAHUD.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ANAPlayerCharacter::ANAPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = StandingArmLength;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f); // Over the right shoulder.
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 14.f;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(DefaultFieldOfView);

	SurvivalComponent = CreateDefaultSubobject<UNASurvivalComponent>(TEXT("Survival"));
	InventoryComponent = CreateDefaultSubobject<UNAInventoryComponent>(TEXT("Inventory"));
	AbilityComponent = CreateDefaultSubobject<UNAAbilityComponent>(TEXT("Ability"));
	GetHealthComponent()->SetCanBeDowned(true); // Players go down before dying; teammates can revive.

	// Swimming uses the movement component's flying mode (predicted and replicated like walking).
	GetCharacterMovement()->MaxFlySpeed = SwimSpeed;
	GetCharacterMovement()->BrakingDecelerationFlying = 1200.f;

	// Every player starts with a pistol; the rest are found as pickups.
	DefaultWeapons.Add(ANAWeapon_Pistol::StaticClass());
}

void ANAPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		UpdateCamera(DeltaSeconds);
		UpdateInteractionFocus();
		UpdateBuildPreview();
	}

	if (HasAuthority())
	{
		UpdateFootstepNoise();
	}

	UpdateSwimming();
	UpdateRescue(DeltaSeconds);
}

void ANAPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ANAPlayerCharacter, bAdminFlying, COND_OwnerOnly);
	DOREPLIFETIME(ANAPlayerCharacter, ReviveProgress);
	DOREPLIFETIME(ANAPlayerCharacter, RecoveringUntil);
	DOREPLIFETIME(ANAPlayerCharacter, CarriedBy);
	DOREPLIFETIME(ANAPlayerCharacter, CarriedPlayer);
}

void ANAPlayerCharacter::SetAdminFlying(bool bNewFlying)
{
	if (HasAuthority())
	{
		bAdminFlying = bNewFlying;
		OnRep_AdminFlying();
	}
}

void ANAPlayerCharacter::OnRep_AdminFlying()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (bAdminFlying)
	{
		SetProne(false);
		Move->SetMovementMode(MOVE_Flying);
		Move->MaxFlySpeed = 1200.f;
	}
	else if (Move->MovementMode == MOVE_Flying)
	{
		Move->SetMovementMode(MOVE_Falling);
		Move->MaxFlySpeed = SwimSpeed;
	}
}

// ----- Water -------------------------------------------------------------------------------------

bool ANAPlayerCharacter::IsSwimming() const
{
	return GetCharacterMovement()->MovementMode == MOVE_Flying;
}

bool ANAPlayerCharacter::IsInWater() const
{
	float SurfaceZ = 0.f;
	return ANAWaterVolume::FindWaterAt(GetWorld(), GetActorLocation(), SurfaceZ) && GetActorLocation().Z - 60.f < SurfaceZ;
}

bool ANAPlayerCharacter::IsHeadUnderwater() const
{
	float SurfaceZ = 0.f;
	const FVector Eyes = GetPawnViewLocation();
	return ANAWaterVolume::FindWaterAt(GetWorld(), Eyes, SurfaceZ) && Eyes.Z < SurfaceZ;
}

bool ANAPlayerCharacter::IsCameraUnderwater() const
{
	float SurfaceZ = 0.f;
	const FVector CameraLocation = FollowCamera->GetComponentLocation();
	return ANAWaterVolume::FindWaterAt(GetWorld(), CameraLocation, SurfaceZ) && CameraLocation.Z < SurfaceZ;
}

void ANAPlayerCharacter::UpdateSwimming()
{
	// The server and the owning client both decide from the same position, so they agree.
	if ((!HasAuthority() && !IsLocallyControlled()) || CarriedBy)
	{
		return;
	}

	UCharacterMovementComponent* Move = GetCharacterMovement();
	float SurfaceZ = 0.f;
	const bool bInWaterArea = ANAWaterVolume::FindWaterAt(GetWorld(), GetActorLocation(), SurfaceZ);
	const float CenterZ = GetActorLocation().Z;
	const bool bSwimmingNow = Move->MovementMode == MOVE_Flying;

	if (IsDead())
	{
		if (bSwimmingNow) { Move->SetMovementMode(MOVE_Falling); }
		return;
	}

	if (bAdminFlying)
	{
		// Admin flight: stay airborne; Space/C move straight up/down.
		if (!bSwimmingNow) { Move->SetMovementMode(MOVE_Flying); }
		Move->MaxFlySpeed = IsSprinting() ? 2400.f : 1200.f;
		if (IsLocallyControlled())
		{
			const float Vertical = (bSwimAscend ? 1.f : 0.f) - (bSwimDescend ? 1.f : 0.f);
			if (!FMath::IsNearlyZero(Vertical)) { AddMovementInput(FVector::UpVector, Vertical); }
		}
		return;
	}

	if (!bSwimmingNow && bInWaterArea && CenterZ < SurfaceZ - 30.f)
	{
		SetProne(false);
		if (bIsCrouched) { UnCrouch(); }
		Move->SetMovementMode(MOVE_Flying);
	}
	else if (bSwimmingNow && (!bInWaterArea || CenterZ > SurfaceZ + 40.f))
	{
		Move->SetMovementMode(MOVE_Falling);
	}

	if (Move->MovementMode != MOVE_Flying)
	{
		return;
	}

	Move->MaxFlySpeed = IsSprinting() ? FastSwimSpeed : SwimSpeed;

	// Float at the surface: you can't swim up out of the water (climb out with Space at the edge).
	if (CenterZ > SurfaceZ - 25.f && Move->Velocity.Z > 0.f)
	{
		Move->Velocity.Z = 0.f;
	}

	if (IsLocallyControlled())
	{
		const float Vertical = (bSwimAscend ? 1.f : 0.f) - (bSwimDescend ? 1.f : 0.f);
		if (!FMath::IsNearlyZero(Vertical))
		{
			AddMovementInput(FVector::UpVector, Vertical);
		}
	}
}

// ----- Vault / climb (mantle) --------------------------------------------------------------------

bool ANAPlayerCharacter::FindMantle(FVector& OutLaunchVelocity) const
{
	if (IsDead() || bIsProne)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const FVector Location = GetActorLocation();
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float FeetZ = Location.Z - HalfHeight;
	FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	if (!GetLastMovementInputVector().IsNearlyZero())
	{
		Forward = GetLastMovementInputVector().GetSafeNormal2D();
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NAMantle), false, this);

	// 1) Something solid right in front, at knee-to-chest height.
	FHitResult WallHit;
	const FVector WallStart(Location.X, Location.Y, FeetZ + 55.f);
	if (!World->LineTraceSingleByChannel(WallHit, WallStart, WallStart + Forward * (Radius + 70.f), ECC_Visibility, Params) ||
		FMath::Abs(WallHit.ImpactNormal.Z) > 0.3f)
	{
		return false;
	}

	// 2) Find its top by tracing down just beyond the wall face.
	const FVector Over = WallHit.ImpactPoint + Forward * 30.f;
	FHitResult TopHit;
	if (!World->LineTraceSingleByChannel(TopHit, FVector(Over.X, Over.Y, FeetZ + MaxMantleHeight + 20.f), FVector(Over.X, Over.Y, FeetZ + 30.f), ECC_Visibility, Params) ||
		TopHit.ImpactNormal.Z < 0.7f)
	{
		return false;
	}

	const float Height = TopHit.ImpactPoint.Z - FeetZ;
	if (Height < 45.f || Height > MaxMantleHeight)
	{
		return false;
	}

	// 3) Room to crouch on top.
	if (World->OverlapBlockingTestByChannel(TopHit.ImpactPoint + FVector(0.f, 0.f, 60.f), FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(Radius * 0.9f, 55.f), Params))
	{
		return false;
	}

	// Launch just high enough to clear it: low obstacles are vaulted over, tall ones climbed onto.
	const float Gravity = FMath::Abs(GetCharacterMovement()->GetGravityZ());
	const float UpSpeed = FMath::Sqrt(2.f * Gravity * (Height + 40.f));
	const float ForwardSpeed = Height < 140.f ? 420.f : 230.f;
	OutLaunchVelocity = Forward * ForwardSpeed + FVector(0.f, 0.f, UpSpeed);
	return true;
}

void ANAPlayerCharacter::ExecuteMantle(const FVector& LaunchVelocity)
{
	if (IsSwimming())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling); // Climbing out of the water.
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	LaunchCharacter(LaunchVelocity, true, true);
}

void ANAPlayerCharacter::ServerMantle_Implementation()
{
	// The server re-checks the obstacle itself; a client can't invent one.
	FVector Launch;
	if (FindMantle(Launch) && (!SurvivalComponent || SurvivalComponent->HasStamina(MantleStaminaCost)))
	{
		if (SurvivalComponent) { SurvivalComponent->ConsumeStamina(MantleStaminaCost); }
		ExecuteMantle(Launch);
	}
}

// ----- Slide -------------------------------------------------------------------------------------

bool ANAPlayerCharacter::CanSlide() const
{
	return !bSliding && !IsDead() && !IsDowned() && IsSprinting() && GetCharacterMovement()->IsMovingOnGround() && GetVelocity().Size2D() > RunSpeed;
}

void ANAPlayerCharacter::StartSlide()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	bSliding = true;
	bSprintHeld = false;
	SetDesiredGait(ENAGait::Run);
	Crouch();

	// Keep momentum: a boost along the current direction, and very little friction for a moment.
	const FVector Direction = GetVelocity().GetSafeNormal2D();
	LaunchCharacter(Direction * (GetVelocity().Size2D() + SlideBoost), true, false);
	DefaultGroundFriction = Move->GroundFriction;
	DefaultBrakingDeceleration = Move->BrakingDecelerationWalking;
	Move->GroundFriction = 0.4f;
	Move->BrakingDecelerationWalking = 250.f;
	GetWorldTimerManager().SetTimer(SlideTimer, this, &ANAPlayerCharacter::EndSlide, SlideSeconds, false);
}

void ANAPlayerCharacter::EndSlide()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->GroundFriction = DefaultGroundFriction;
	Move->BrakingDecelerationWalking = DefaultBrakingDeceleration;
	bSliding = false;
}

void ANAPlayerCharacter::ServerSlide_Implementation()
{
	if (CanSlide())
	{
		StartSlide();
	}
}

// ----- Dodge roll --------------------------------------------------------------------------------

bool ANAPlayerCharacter::CanDodge() const
{
	return !IsDead() && !IsDowned() && !CarriedPlayer && !bIsProne && !IsSwimming() && GetCharacterMovement()->IsMovingOnGround() &&
		GetWorld()->GetTimeSeconds() - LastDodgeTime >= DodgeCooldownSeconds &&
		(!SurvivalComponent || SurvivalComponent->HasStamina(DodgeStaminaCost));
}

void ANAPlayerCharacter::ExecuteDodge(const FVector& Direction)
{
	LastDodgeTime = GetWorld()->GetTimeSeconds();
	LaunchCharacter(Direction.GetSafeNormal2D() * DodgeSpeed + FVector(0.f, 0.f, 180.f), true, true);
}

void ANAPlayerCharacter::ServerDodge_Implementation(FVector_NetQuantizeNormal Direction)
{
	if (!CanDodge() || Direction.IsNearlyZero())
	{
		return;
	}
	if (SurvivalComponent) { SurvivalComponent->ConsumeStamina(DodgeStaminaCost); }
	GrantInvulnerability(0.35f); // A well-timed roll avoids a hit.
	ExecuteDodge(Direction);
}

void ANAPlayerCharacter::UpdateFootstepNoise()
{
	const float Speed = GetVelocity().Size2D();
	if (IsDead() || Speed < 150.f || GetCharacterMovement()->IsFalling())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextFootstepNoiseTime)
	{
		return;
	}

	// Sprinting is loud, running audible, walking and crouching nearly silent.
	float Loudness = 0.5f;
	float Range = 900.f;
	if (IsSprinting())             { Loudness = 1.f;  Range = 1600.f; }
	else if (bIsCrouched)          { Loudness = 0.1f; Range = 250.f; }
	else if (Gait == ENAGait::Walk) { Loudness = 0.2f; Range = 400.f; }

	if (const ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this))
	{
		Range *= Environment->GetNoiseScale();
	}

	NextFootstepNoiseTime = Now + 0.5f;
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), Loudness, this, Range, FName(TEXT("Footstep")));
}

void ANAPlayerCharacter::UpdateCamera(float DeltaSeconds)
{
	const ANAWeapon* Weapon = GetCurrentWeapon();
	const bool bAimingWeapon = IsAiming() && Weapon;

	float TargetLength = StandingArmLength;
	float TargetFieldOfView = DefaultFieldOfView;
	if (bAimingWeapon)
	{
		TargetLength = Weapon->GetStats().AimArmLength;
		TargetFieldOfView = Weapon->GetStats().AimFieldOfView;
	}
	else if (IsSprinting() && GetVelocity().Size2D() > RunSpeed)
	{
		TargetLength = SprintArmLength;
	}

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetLength, DeltaSeconds, CameraInterpSpeed);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFieldOfView, DeltaSeconds, CameraInterpSpeed));
}

void ANAPlayerCharacter::UpdateInteractionFocus()
{
	FocusedInteractable = nullptr;
	GatherPrompt.Reset();
	if (IsDead() || IsDowned() || CarriedPlayer)
	{
		return;
	}

	const FVector Start = FollowCamera->GetComponentLocation();
	const FVector End = Start + FollowCamera->GetForwardVector() * (CameraBoom->TargetArmLength + InteractionRange);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NAInteractFocus), false, this);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return;
	}

	AActor* Candidate = Hit.GetActor();
	const INAInteractable* Interactable = Cast<INAInteractable>(Candidate);
	if (Interactable && FVector::Dist(Candidate->GetActorLocation(), GetActorLocation()) <= InteractionRange && Interactable->CanInteract(this))
	{
		FocusedInteractable = Candidate;
		return;
	}
	UpdateGatherFocus(Hit);
}

void ANAPlayerCharacter::ServerInteract_Implementation(AActor* Target)
{
	INAInteractable* Interactable = Cast<INAInteractable>(Target);
	if (!Interactable || IsDead() || IsDowned() || Target == this)
	{
		return;
	}
	// Anti-cheat: the server re-checks range (with slack for latency) and availability.
	if (FVector::Dist(Target->GetActorLocation(), GetActorLocation()) > InteractionRange + 150.f || !Interactable->CanInteract(this))
	{
		return;
	}
	Interactable->Interact(this);
}

bool ANAPlayerCharacter::CanSprint() const
{
	return !IsAiming() && !IsDowned() && !CarriedPlayer && (!SurvivalComponent || SurvivalComponent->CanSprint());
}

void ANAPlayerCharacter::GetAimViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	OutLocation = FollowCamera->GetComponentLocation();
	OutRotation = GetControlRotation();
}

void ANAPlayerCharacter::ApplyRecoil(float PitchDegrees, float YawDegrees)
{
	if (IsLocallyControlled() && Controller)
	{
		FRotator Rotation = Controller->GetControlRotation();
		Rotation.Pitch += PitchDegrees;
		Rotation.Yaw += FMath::FRandRange(-YawDegrees, YawDegrees);
		Controller->SetControlRotation(Rotation);
	}
}

void ANAPlayerCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	if (HasAuthority() && SurvivalComponent)
	{
		SurvivalComponent->ConsumeStamina(JumpStaminaCost);
	}
}

// ----- Input ---------------------------------------------------------------------------------

void ANAPlayerCharacter::BuildDefaultInputIfNeeded()
{
	if (MappingContext)
	{
		return; // Designer-authored input in use.
	}

	auto MakeAction = [this](const TCHAR* ActionName, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, ActionName, RF_Transient);
		Action->ValueType = Type;
		return Action;
	};

	MoveAction           = MakeAction(TEXT("IA_Move"),           EInputActionValueType::Axis2D);
	LookAction           = MakeAction(TEXT("IA_Look"),           EInputActionValueType::Axis2D);
	JumpAction           = MakeAction(TEXT("IA_Jump"),           EInputActionValueType::Boolean);
	SprintAction         = MakeAction(TEXT("IA_Sprint"),         EInputActionValueType::Boolean);
	WalkAction           = MakeAction(TEXT("IA_Walk"),           EInputActionValueType::Boolean);
	CrouchAction         = MakeAction(TEXT("IA_Crouch"),         EInputActionValueType::Boolean);
	FireAction           = MakeAction(TEXT("IA_Fire"),           EInputActionValueType::Boolean);
	AimAction            = MakeAction(TEXT("IA_Aim"),            EInputActionValueType::Boolean);
	ReloadAction         = MakeAction(TEXT("IA_Reload"),         EInputActionValueType::Boolean);
	InteractAction       = MakeAction(TEXT("IA_Interact"),       EInputActionValueType::Boolean);
	NextWeaponAction     = MakeAction(TEXT("IA_NextWeapon"),     EInputActionValueType::Boolean);
	PreviousWeaponAction = MakeAction(TEXT("IA_PreviousWeapon"), EInputActionValueType::Boolean);
	WeaponSlotActions.Reset();
	WeaponSlotActions.Add(MakeAction(TEXT("IA_WeaponSlot1"), EInputActionValueType::Boolean));
	WeaponSlotActions.Add(MakeAction(TEXT("IA_WeaponSlot2"), EInputActionValueType::Boolean));
	WeaponSlotActions.Add(MakeAction(TEXT("IA_WeaponSlot3"), EInputActionValueType::Boolean));
	WeaponSlotActions.Add(MakeAction(TEXT("IA_WeaponSlot4"), EInputActionValueType::Boolean));
	WeaponSlotActions.Add(MakeAction(TEXT("IA_WeaponSlot5"), EInputActionValueType::Boolean));
	ProneAction = MakeAction(TEXT("IA_Prone"), EInputActionValueType::Boolean);
	DodgeAction = MakeAction(TEXT("IA_Dodge"), EInputActionValueType::Boolean);
	ScoreboardAction = MakeAction(TEXT("IA_Scoreboard"), EInputActionValueType::Boolean);
	HelpAction = MakeAction(TEXT("IA_Help"), EInputActionValueType::Boolean);
	InventoryAction = MakeAction(TEXT("IA_Inventory"), EInputActionValueType::Boolean);
	MenuUpAction = MakeAction(TEXT("IA_MenuUp"), EInputActionValueType::Boolean);
	MenuDownAction = MakeAction(TEXT("IA_MenuDown"), EInputActionValueType::Boolean);
	MenuLeftAction = MakeAction(TEXT("IA_MenuLeft"), EInputActionValueType::Boolean);
	MenuRightAction = MakeAction(TEXT("IA_MenuRight"), EInputActionValueType::Boolean);
	MenuConfirmAction = MakeAction(TEXT("IA_MenuConfirm"), EInputActionValueType::Boolean);
	AbilityAction = MakeAction(TEXT("IA_Ability"), EInputActionValueType::Boolean);

	UInputMappingContext* IMC = NewObject<UInputMappingContext>(this, TEXT("IMC_NullActionDefault"), RF_Transient);

	auto Swizzle = [IMC]() { return NewObject<UInputModifierSwizzleAxis>(IMC); }; // Default order YXZ: key value goes to Y.
	auto Negate = [IMC](bool bNegateX, bool bNegateY)
	{
		UInputModifierNegate* Mod = NewObject<UInputModifierNegate>(IMC);
		Mod->bX = bNegateX;
		Mod->bY = bNegateY;
		Mod->bZ = false;
		return Mod;
	};

	// Move: Y = forward/back, X = right/left.
	IMC->MapKey(MoveAction, EKeys::W).Modifiers.Add(Swizzle());
	{
		FEnhancedActionKeyMapping& Back = IMC->MapKey(MoveAction, EKeys::S);
		Back.Modifiers.Add(Swizzle());
		Back.Modifiers.Add(Negate(true, true));
	}
	IMC->MapKey(MoveAction, EKeys::A).Modifiers.Add(Negate(true, true));
	IMC->MapKey(MoveAction, EKeys::D);
	IMC->MapKey(MoveAction, EKeys::Gamepad_Left2D).Modifiers.Add(NewObject<UInputModifierDeadZone>(IMC));

	// Look: mouse delta, inverted Y so pushing the mouse forward looks up.
	IMC->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(Negate(false, true));
	{
		FEnhancedActionKeyMapping& Stick = IMC->MapKey(LookAction, EKeys::Gamepad_Right2D);
		Stick.Modifiers.Add(NewObject<UInputModifierDeadZone>(IMC));
		Stick.Modifiers.Add(Negate(false, true));
		UInputModifierScalar* Rate = NewObject<UInputModifierScalar>(IMC);
		Rate->Scalar = FVector(60.f, 45.f, 1.f); // Stick deflection -> degrees/sec (before engine look scale).
		Stick.Modifiers.Add(Rate);
		Stick.Modifiers.Add(NewObject<UInputModifierScaleByDeltaTime>(IMC));
	}

	IMC->MapKey(JumpAction, EKeys::SpaceBar);
	IMC->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);

	IMC->MapKey(SprintAction, EKeys::LeftShift);
	IMC->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);

	IMC->MapKey(WalkAction, EKeys::LeftAlt);
	IMC->MapKey(WalkAction, EKeys::Gamepad_DPad_Down);

	IMC->MapKey(CrouchAction, EKeys::C);
	IMC->MapKey(CrouchAction, EKeys::LeftControl);
	IMC->MapKey(CrouchAction, EKeys::Gamepad_FaceButton_Right);

	IMC->MapKey(FireAction, EKeys::LeftMouseButton);
	IMC->MapKey(FireAction, EKeys::Gamepad_RightTrigger);

	IMC->MapKey(AimAction, EKeys::RightMouseButton);
	IMC->MapKey(AimAction, EKeys::Gamepad_LeftTrigger);

	IMC->MapKey(ReloadAction, EKeys::R);
	IMC->MapKey(ReloadAction, EKeys::Gamepad_FaceButton_Left);

	IMC->MapKey(InteractAction, EKeys::E);
	IMC->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Top);

	IMC->MapKey(NextWeaponAction, EKeys::MouseScrollUp);
	IMC->MapKey(NextWeaponAction, EKeys::Gamepad_RightShoulder);
	IMC->MapKey(PreviousWeaponAction, EKeys::MouseScrollDown);
	IMC->MapKey(PreviousWeaponAction, EKeys::Gamepad_LeftShoulder);

	IMC->MapKey(WeaponSlotActions[0], EKeys::One);
	IMC->MapKey(WeaponSlotActions[1], EKeys::Two);
	IMC->MapKey(WeaponSlotActions[2], EKeys::Three);
	IMC->MapKey(WeaponSlotActions[3], EKeys::Four);
	if (WeaponSlotActions.Num() >= 5) { IMC->MapKey(WeaponSlotActions[4], EKeys::Five); }

	IMC->MapKey(ProneAction, EKeys::Z);
	IMC->MapKey(ProneAction, EKeys::Gamepad_DPad_Up);
	IMC->MapKey(DodgeAction, EKeys::V);
	IMC->MapKey(DodgeAction, EKeys::Gamepad_RightThumbstick);
	IMC->MapKey(ScoreboardAction, EKeys::Tab);
	IMC->MapKey(HelpAction, EKeys::F1);
	IMC->MapKey(ScoreboardAction, EKeys::Gamepad_Special_Left);
	IMC->MapKey(InventoryAction, EKeys::I);
	IMC->MapKey(InventoryAction, EKeys::Gamepad_Special_Right);
	IMC->MapKey(MenuUpAction, EKeys::Up);
	IMC->MapKey(MenuDownAction, EKeys::Down);
	IMC->MapKey(MenuLeftAction, EKeys::Left);
	IMC->MapKey(MenuRightAction, EKeys::Right);
	IMC->MapKey(MenuConfirmAction, EKeys::Enter);
	IMC->MapKey(AbilityAction, EKeys::Q);

	MappingContext = IMC;
}

void ANAPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildDefaultInputIfNeeded();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogNullAction, Error, TEXT("Enhanced Input is not the default input component. Check Config/DefaultInput.ini."));
		return;
	}

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Input_JumpStarted);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::Input_JumpCompleted);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ThisClass::Input_SprintStarted);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::Input_SprintCompleted);
	Input->BindAction(WalkAction, ETriggerEvent::Started, this, &ThisClass::Input_WalkToggle);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::Input_Crouch);
	Input->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ThisClass::Input_CrouchCompleted);
	if (ProneAction) { Input->BindAction(ProneAction, ETriggerEvent::Started, this, &ThisClass::Input_Prone); }
	if (DodgeAction) { Input->BindAction(DodgeAction, ETriggerEvent::Started, this, &ThisClass::Input_Dodge); }
	if (InventoryAction)   { Input->BindAction(InventoryAction, ETriggerEvent::Started, this, &ThisClass::Input_Inventory); }
	if (MenuUpAction)      { Input->BindAction(MenuUpAction, ETriggerEvent::Started, this, &ThisClass::Input_MenuUp); }
	if (MenuDownAction)    { Input->BindAction(MenuDownAction, ETriggerEvent::Started, this, &ThisClass::Input_MenuDown); }
	if (MenuLeftAction)    { Input->BindAction(MenuLeftAction, ETriggerEvent::Started, this, &ThisClass::Input_MenuLeft); }
	if (MenuRightAction)   { Input->BindAction(MenuRightAction, ETriggerEvent::Started, this, &ThisClass::Input_MenuRight); }
	if (MenuConfirmAction) { Input->BindAction(MenuConfirmAction, ETriggerEvent::Started, this, &ThisClass::Input_MenuConfirm); }
	if (AbilityAction)     { Input->BindAction(AbilityAction, ETriggerEvent::Started, this, &ThisClass::Input_Ability); }
	if (HelpAction)        { Input->BindAction(HelpAction, ETriggerEvent::Started, this, &ThisClass::Input_Help); }
	if (ScoreboardAction)
	{
		Input->BindAction(ScoreboardAction, ETriggerEvent::Started, this, &ThisClass::Input_ScoreboardStarted);
		Input->BindAction(ScoreboardAction, ETriggerEvent::Completed, this, &ThisClass::Input_ScoreboardCompleted);
	}
	Input->BindAction(FireAction, ETriggerEvent::Started, this, &ThisClass::Input_FireStarted);
	Input->BindAction(FireAction, ETriggerEvent::Completed, this, &ThisClass::Input_FireCompleted);
	Input->BindAction(AimAction, ETriggerEvent::Started, this, &ThisClass::Input_AimStarted);
	Input->BindAction(AimAction, ETriggerEvent::Completed, this, &ThisClass::Input_AimCompleted);
	Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &ThisClass::Input_Reload);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &ThisClass::Input_Interact);
	Input->BindAction(NextWeaponAction, ETriggerEvent::Started, this, &ThisClass::Input_NextWeapon);
	Input->BindAction(PreviousWeaponAction, ETriggerEvent::Started, this, &ThisClass::Input_PreviousWeapon);

	if (WeaponSlotActions.Num() >= 4)
	{
		Input->BindAction(WeaponSlotActions[0], ETriggerEvent::Started, this, &ThisClass::Input_WeaponSlot1);
		Input->BindAction(WeaponSlotActions[1], ETriggerEvent::Started, this, &ThisClass::Input_WeaponSlot2);
		Input->BindAction(WeaponSlotActions[2], ETriggerEvent::Started, this, &ThisClass::Input_WeaponSlot3);
		Input->BindAction(WeaponSlotActions[3], ETriggerEvent::Started, this, &ThisClass::Input_WeaponSlot4);
	}
	if (WeaponSlotActions.Num() >= 5)
	{
		Input->BindAction(WeaponSlotActions[4], ETriggerEvent::Started, this, &ThisClass::Input_WeaponSlot5);
	}
}

void ANAPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	BuildDefaultInputIfNeeded();

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			// After a respawn the previous body's mapping context is stale; start clean.
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

void ANAPlayerCharacter::Input_Move(const FInputActionValue& Value)
{
	if (!Controller || IsDead())
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();

	if (IsSwimming())
	{
		// In water you swim where you look, including up and down.
		const FRotationMatrix LookMatrix(Controller->GetControlRotation());
		AddMovementInput(LookMatrix.GetUnitAxis(EAxis::X), Axis.Y);
		AddMovementInput(LookMatrix.GetUnitAxis(EAxis::Y), Axis.X);
		return;
	}

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FRotationMatrix YawMatrix(YawRotation);

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void ANAPlayerCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ANAPlayerCharacter::Input_JumpStarted()
{
	if (IsDead() || IsDowned() || CarriedPlayer)
	{
		return;
	}

	FVector MantleLaunch;
	const bool bCanMantle = FindMantle(MantleLaunch) && (!SurvivalComponent || SurvivalComponent->HasStamina(MantleStaminaCost));

	if (IsSwimming() && !bCanMantle)
	{
		bSwimAscend = true; // Hold Space to swim up.
		return;
	}
	if (bCanMantle)
	{
		ExecuteMantle(MantleLaunch); // Predicted locally; the server checks and does the same.
		if (!HasAuthority()) { ServerMantle(); }
		else if (SurvivalComponent) { SurvivalComponent->ConsumeStamina(MantleStaminaCost); }
		return;
	}
	if (bIsProne)
	{
		SetProne(false); // Prone -> crouch.
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch(); // Jump from crouch stands up first, like most shooters.
		return;
	}
	if (SurvivalComponent && !SurvivalComponent->HasStamina(JumpStaminaCost))
	{
		return; // Too exhausted to jump.
	}
	Jump();
}

void ANAPlayerCharacter::Input_JumpCompleted()
{
	bSwimAscend = false;
	StopJumping();
}

void ANAPlayerCharacter::Input_SprintStarted()
{
	bSprintHeld = true;
	UpdateDesiredGait();
}

void ANAPlayerCharacter::Input_SprintCompleted()
{
	bSprintHeld = false;
	UpdateDesiredGait();
}

void ANAPlayerCharacter::Input_WalkToggle()
{
	bWalkToggled = !bWalkToggled;
	UpdateDesiredGait();
}

void ANAPlayerCharacter::Input_Crouch()
{
	if (IsDowned())
	{
		return; // Already crawling.
	}
	if (IsSwimming())
	{
		bSwimDescend = true; // Hold C to dive.
		return;
	}
	if (CanSlide())
	{
		// Tell the server first (while it still sees us sprinting), then predict locally.
		if (!HasAuthority()) { ServerSlide(); }
		StartSlide(); // Crouch while sprinting = slide.
		return;
	}
	ToggleCrouch();
}

void ANAPlayerCharacter::Input_CrouchCompleted()
{
	bSwimDescend = false;
}

void ANAPlayerCharacter::Input_Help()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (ANAHUD* GameHUD = PC ? Cast<ANAHUD>(PC->GetHUD()) : nullptr)
	{
		GameHUD->ToggleHelp();
	}
}

void ANAPlayerCharacter::Input_ScoreboardStarted()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (ANAHUD* GameHUD = PC ? Cast<ANAHUD>(PC->GetHUD()) : nullptr)
	{
		GameHUD->SetScoreboardVisible(true);
	}
}

void ANAPlayerCharacter::Input_ScoreboardCompleted()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (ANAHUD* GameHUD = PC ? Cast<ANAHUD>(PC->GetHUD()) : nullptr)
	{
		GameHUD->SetScoreboardVisible(false);
	}
}

void ANAPlayerCharacter::Input_Prone()
{
	if (IsDowned() || CarriedPlayer)
	{
		return;
	}
	SetProne(!IsProne());
}

void ANAPlayerCharacter::Input_Dodge()
{
	if (!CanDodge())
	{
		return;
	}
	// Roll in the direction you're steering; backwards if you aren't moving.
	FVector Direction = GetLastMovementInputVector().GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		Direction = -GetActorForwardVector().GetSafeNormal2D();
	}
	ExecuteDodge(Direction);
	if (!HasAuthority())
	{
		ServerDodge(Direction);
	}
	else
	{
		if (SurvivalComponent) { SurvivalComponent->ConsumeStamina(DodgeStaminaCost); }
		GrantInvulnerability(0.35f);
	}
}

void ANAPlayerCharacter::UpdateDesiredGait()
{
	if (bSprintHeld)
	{
		SetDesiredGait(ENAGait::Sprint);
	}
	else
	{
		SetDesiredGait(bWalkToggled ? ENAGait::Walk : ENAGait::Run);
	}
}

void ANAPlayerCharacter::Input_FireStarted()
{
	if (IsDowned() || CarriedPlayer)
	{
		return;
	}
	if (bBuildMode)
	{
		if (bPlacementValid)
		{
			ServerPlace(BuildItem, BuildAimPoint, BuildYaw); // The server re-checks everything.
		}
		return;
	}
	if (ANAWeapon* Weapon = GetCurrentWeapon())
	{
		if (IsSprinting())
		{
			bSprintHeld = false;
			SetDesiredGait(bWalkToggled ? ENAGait::Walk : ENAGait::Run); // Firing stops a sprint.
		}
		Weapon->StartFire();
	}
}

void ANAPlayerCharacter::Input_FireCompleted()
{
	if (ANAWeapon* Weapon = GetCurrentWeapon())
	{
		Weapon->StopFire();
	}
}

void ANAPlayerCharacter::Input_AimStarted()
{
	if (IsDowned() || CarriedPlayer)
	{
		return;
	}
	if (bBuildMode)
	{
		StopBuilding(); // Right-click cancels building.
		return;
	}
	SetAiming(true);
}

void ANAPlayerCharacter::Input_AimCompleted()
{
	SetAiming(false);
}

void ANAPlayerCharacter::Input_Reload()
{
	if (bBuildMode)
	{
		BuildYaw = FMath::Fmod(BuildYaw + 90.f, 360.f); // R rotates the piece while building.
		return;
	}
	if (ANAWeapon* Weapon = GetCurrentWeapon())
	{
		Weapon->RequestReload();
	}
}

void ANAPlayerCharacter::Input_Ability()
{
	if (AbilityComponent && !IsDowned() && !CarriedPlayer && !bInventoryOpen)
	{
		AbilityComponent->RequestActivate(); // The server checks energy, cooldown and state.
	}
}

void ANAPlayerCharacter::Input_Interact()
{
	if (CarriedPlayer)
	{
		ServerDropCarried(); // Put the carried teammate down.
		return;
	}
	if (IsDowned())
	{
		return;
	}
	if (AActor* Target = FocusedInteractable.Get())
	{
		ServerInteract(Target);
	}
	else if (!GatherPrompt.IsEmpty())
	{
		ServerGather(GatherLocation);
	}
}

void ANAPlayerCharacter::Input_NextWeapon()     { CycleWeapon(+1); }
void ANAPlayerCharacter::Input_PreviousWeapon() { CycleWeapon(-1); }
void ANAPlayerCharacter::Input_WeaponSlot1()    { EquipWeaponInSlot(1); }
void ANAPlayerCharacter::Input_WeaponSlot2()    { EquipWeaponInSlot(2); }
void ANAPlayerCharacter::Input_WeaponSlot3()    { EquipWeaponInSlot(3); }
void ANAPlayerCharacter::Input_WeaponSlot4()    { EquipWeaponInSlot(4); }
void ANAPlayerCharacter::Input_WeaponSlot5()    { EquipWeaponInSlot(5); }
