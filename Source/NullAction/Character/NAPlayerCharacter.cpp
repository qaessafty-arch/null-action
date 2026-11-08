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
#include "Survival/NASurvivalComponent.h"
#include "Weapons/NAWeapon.h"
#include "Weapons/NAWeaponTypes.h"

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
	}
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
	if (IsDead())
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
	}
}

void ANAPlayerCharacter::ServerInteract_Implementation(AActor* Target)
{
	INAInteractable* Interactable = Cast<INAInteractable>(Target);
	if (!Interactable || IsDead())
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
	return !IsAiming() && (!SurvivalComponent || SurvivalComponent->CanSprint());
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
	if (IsDead())
	{
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
	ToggleCrouch();
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
	SetAiming(true);
}

void ANAPlayerCharacter::Input_AimCompleted()
{
	SetAiming(false);
}

void ANAPlayerCharacter::Input_Reload()
{
	if (ANAWeapon* Weapon = GetCurrentWeapon())
	{
		Weapon->RequestReload();
	}
}

void ANAPlayerCharacter::Input_Interact()
{
	if (AActor* Target = FocusedInteractable.Get())
	{
		ServerInteract(Target);
	}
}

void ANAPlayerCharacter::Input_NextWeapon()     { CycleWeapon(+1); }
void ANAPlayerCharacter::Input_PreviousWeapon() { CycleWeapon(-1); }
void ANAPlayerCharacter::Input_WeaponSlot1()    { EquipWeaponInSlot(1); }
void ANAPlayerCharacter::Input_WeaponSlot2()    { EquipWeaponInSlot(2); }
void ANAPlayerCharacter::Input_WeaponSlot3()    { EquipWeaponInSlot(3); }
void ANAPlayerCharacter::Input_WeaponSlot4()    { EquipWeaponInSlot(4); }
