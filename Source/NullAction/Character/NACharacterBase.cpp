#include "Character/NACharacterBase.h"

#include "NullAction.h"
#include "Animation/AnimInstance.h"
#include "Combat/NACombatTypes.h"
#include "Combat/NAHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAGameMode.h"
#include "Engine/DamageEvents.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/PackageName.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/NAWeapon.h"

namespace NACharacter
{
	// Engine basic shapes are 100 units across; the cylinder is centred on its pivot.
	constexpr float ShapeSize = 100.f;
	constexpr float CapsuleRadius = 35.f;
	constexpr float CapsuleHalfHeight = 90.f;
}

ANACharacterBase::ANACharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->InitCapsuleSize(NACharacter::CapsuleRadius, NACharacter::CapsuleHalfHeight);

	// Third-person action setup: the body turns toward its movement, not toward the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->MaxWalkSpeed = RunSpeed;
	Move->MaxWalkSpeedCrouched = CrouchSpeed;
	Move->MaxAcceleration = 1600.f;
	Move->BrakingDecelerationWalking = 1800.f;
	Move->JumpZVelocity = 460.f;
	Move->AirControl = 0.3f;
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->bCanWalkOffLedgesWhenCrouching = true;
	Move->SetCrouchedHalfHeight(55.f);

	HealthComponent = CreateDefaultSubobject<UNAHealthComponent>(TEXT("Health"));

	// Mannequin-style meshes have their origin at the feet and face +Y.
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -NACharacter::CapsuleHalfHeight), FRotator(0.f, -90.f, 0.f));
	// Keep bones updated on the server too, so server-side hit detection sees real poses.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	BodyMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny")));
	BodyAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	FallbackBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallbackBody"));
	FallbackBody->SetupAttachment(GetCapsuleComponent());
	FallbackBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FallbackBody->SetGenerateOverlapEvents(false);
	if (CylinderMesh.Succeeded())
	{
		FallbackBody->SetStaticMesh(CylinderMesh.Object);
	}

	// A "visor" block on the face so you can see which way the stand-in body is facing.
	FallbackVisor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallbackVisor"));
	FallbackVisor->SetupAttachment(GetCapsuleComponent());
	FallbackVisor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FallbackVisor->SetGenerateOverlapEvents(false);
	FallbackVisor->SetRelativeScale3D(FVector(0.12f, 0.4f, 0.12f));
	if (CubeMesh.Succeeded())
	{
		FallbackVisor->SetStaticMesh(CubeMesh.Object);
	}

	UpdateFallbackBodyHeight();
}

void ANACharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANACharacterBase, Gait);
	DOREPLIFETIME(ANACharacterBase, Weapons);
	DOREPLIFETIME(ANACharacterBase, CurrentWeapon);
	DOREPLIFETIME(ANACharacterBase, bIsAiming);
}

void ANACharacterBase::BeginPlay()
{
	Super::BeginPlay();

	DefaultMeshRelativeLocation = GetMesh()->GetRelativeLocation();
	DefaultMeshRelativeRotation = GetMesh()->GetRelativeRotation();
	DefaultMeshCollisionProfile = GetMesh()->GetCollisionProfileName();

	HealthComponent->OnDeathStateChanged.AddDynamic(this, &ANACharacterBase::HandleDeathStateChanged);
	if (HasAuthority())
	{
		HealthComponent->OnKilled.AddDynamic(this, &ANACharacterBase::HandleKilled);
	}

	ApplyGait();
	ResolveAppearance();

	if (HasAuthority())
	{
		for (const TSubclassOf<ANAWeapon>& WeaponClass : DefaultWeapons)
		{
			GiveWeapon(WeaponClass, /*bEquip*/ false);
		}
	}
}

void ANACharacterBase::Destroyed()
{
	if (HasAuthority())
	{
		for (ANAWeapon* Weapon : Weapons)
		{
			if (Weapon)
			{
				Weapon->Destroy();
			}
		}
		Weapons.Empty();
	}
	Super::Destroyed();
}

// ----- Movement --------------------------------------------------------------------------------

void ANACharacterBase::SetDesiredGait(ENAGait NewGait)
{
	if (NewGait == ENAGait::Sprint && !CanSprint())
	{
		NewGait = ENAGait::Run;
	}

	if (NewGait == ENAGait::Sprint && bIsCrouched)
	{
		UnCrouch();
	}

	if (Gait != NewGait)
	{
		Gait = NewGait;
		ApplyGait();
	}

	if (!HasAuthority())
	{
		ServerSetGait(NewGait);
	}
}

void ANACharacterBase::ServerSetGait_Implementation(ENAGait NewGait)
{
	if (static_cast<uint8>(NewGait) > static_cast<uint8>(ENAGait::Sprint))
	{
		return; // Reject garbage from a modified client.
	}
	SetDesiredGait(NewGait);
}

void ANACharacterBase::OnRep_Gait()
{
	ApplyGait();
}

void ANACharacterBase::ApplyGait()
{
	float Speed = RunSpeed;
	switch (Gait)
	{
	case ENAGait::Walk:   Speed = WalkSpeed;   break;
	case ENAGait::Run:    Speed = RunSpeed;    break;
	case ENAGait::Sprint: Speed = SprintSpeed; break;
	}

	// Aiming slows you down; you can't sprint and aim at once.
	if (bIsAiming)
	{
		Speed = FMath::Min(Speed, RunSpeed) * AimSpeedMultiplier;
	}

	GetCharacterMovement()->MaxWalkSpeed = Speed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = bIsAiming ? CrouchSpeed * AimSpeedMultiplier : CrouchSpeed;
}

void ANACharacterBase::ToggleCrouch()
{
	if (IsDead())
	{
		return;
	}

	if (bIsCrouched)
	{
		UnCrouch();
		return;
	}

	if (Gait == ENAGait::Sprint)
	{
		SetDesiredGait(ENAGait::Run);
	}
	Crouch(); // ACharacter crouch is client-predicted and server-replicated by the movement component.
}

void ANACharacterBase::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	UpdateFallbackBodyHeight();
}

void ANACharacterBase::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	UpdateFallbackBodyHeight();
}

// ----- Health and death ------------------------------------------------------------------------

bool ANACharacterBase::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

float ANACharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Damage <= 0.f || !HealthComponent)
	{
		return 0.f;
	}

	ENAHitZone Zone = ENAHitZone::Body;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		Zone = NACombat::GetHitZone(static_cast<const FPointDamageEvent&>(DamageEvent).HitInfo);
	}
	return HealthComponent->ApplyDamage(Damage, Zone, EventInstigator, DamageCauser);
}

void ANACharacterBase::HandleKilled(AActor* Victim, AController* Killer)
{
	OnKilled(Killer);
}

void ANACharacterBase::OnKilled(AController* Killer)
{
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->OnCharacterKilled(this, Killer);
	}
}

void ANACharacterBase::HandleDeathStateChanged(bool bNowDead)
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	USkeletalMeshComponent* MeshComp = GetMesh();
	const bool bRealBody = HasRealBody();

	if (bNowDead)
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
		bIsAiming = false;
		UpdateRotationMode();

		Move->StopMovementImmediately();
		Move->DisableMovement();

		// The body no longer blocks players or bullets.
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Capsule->SetCollisionResponseToChannel(NA_TRACE_WEAPON, ECR_Ignore);

		if (bRealBody)
		{
			MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
			MeshComp->SetSimulatePhysics(true);
		}
		else
		{
			// Stand-in body tips over onto the ground.
			const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
			FallbackBody->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -HalfHeight + 30.f), FRotator(0.f, 0.f, 90.f));
			FallbackVisor->SetVisibility(false);
		}
	}
	else
	{
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Capsule->SetCollisionResponseToChannel(NA_TRACE_WEAPON, bRealBody ? ECR_Ignore : ECR_Block);
		Move->SetMovementMode(MOVE_Walking);

		if (MeshComp->IsSimulatingPhysics())
		{
			MeshComp->SetSimulatePhysics(false);
			MeshComp->AttachToComponent(Capsule, FAttachmentTransformRules::KeepRelativeTransform);
			MeshComp->SetRelativeLocationAndRotation(DefaultMeshRelativeLocation, DefaultMeshRelativeRotation);
		}
		if (DefaultMeshCollisionProfile != NAME_None)
		{
			MeshComp->SetCollisionProfileName(DefaultMeshCollisionProfile);
		}

		FallbackBody->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		FallbackVisor->SetVisibility(!bRealBody);
		UpdateFallbackBodyHeight();
	}
}

// ----- Weapons ---------------------------------------------------------------------------------

ANAWeapon* ANACharacterBase::GiveWeapon(TSubclassOf<ANAWeapon> WeaponClass, bool bEquip)
{
	if (!HasAuthority() || !WeaponClass || IsDead())
	{
		return nullptr;
	}

	// Already carrying this gun: take its ammo instead.
	for (ANAWeapon* Existing : Weapons)
	{
		if (Existing && Existing->GetClass() == WeaponClass)
		{
			return Existing->AddReserveAmmo(Existing->GetStats().MagazineSize * 2) > 0 ? Existing : nullptr;
		}
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ANAWeapon* Weapon = GetWorld()->SpawnActor<ANAWeapon>(WeaponClass, GetActorTransform(), Params);
	if (!Weapon)
	{
		return nullptr;
	}

	// One weapon per slot: a new gun replaces whatever was in its slot.
	for (int32 Index = Weapons.Num() - 1; Index >= 0; --Index)
	{
		ANAWeapon* Other = Weapons[Index];
		if (Other && Other->GetStats().Slot == Weapon->GetStats().Slot)
		{
			if (CurrentWeapon == Other)
			{
				CurrentWeapon = nullptr;
			}
			Other->Destroy();
			Weapons.RemoveAt(Index);
		}
	}

	Weapons.Add(Weapon);
	AttachWeapon(Weapon);
	Weapon->SetActorHiddenInGame(true);

	if (bEquip || !CurrentWeapon)
	{
		EquipWeapon(Weapon);
	}
	return Weapon;
}

void ANACharacterBase::EquipWeapon(ANAWeapon* Weapon)
{
	if (!Weapon || Weapon == CurrentWeapon || IsDead())
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerEquipWeapon(Weapon);
		return;
	}

	if (!Weapons.Contains(Weapon))
	{
		return; // Can't equip a gun you don't own.
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->OnUnequipped();
	}
	CurrentWeapon = Weapon;
	CurrentWeapon->OnEquipped();
	OnRep_CurrentWeapon();
}

void ANACharacterBase::ServerEquipWeapon_Implementation(ANAWeapon* Weapon)
{
	EquipWeapon(Weapon);
}

void ANACharacterBase::EquipWeaponInSlot(int32 Slot)
{
	for (ANAWeapon* Weapon : Weapons)
	{
		if (Weapon && Weapon->GetStats().Slot == Slot)
		{
			EquipWeapon(Weapon);
			return;
		}
	}
}

void ANACharacterBase::CycleWeapon(int32 Direction)
{
	TArray<ANAWeapon*> Sorted;
	for (ANAWeapon* Weapon : Weapons)
	{
		if (Weapon)
		{
			Sorted.Add(Weapon);
		}
	}
	if (Sorted.Num() < 2)
	{
		return;
	}
	Sorted.Sort([](const ANAWeapon& A, const ANAWeapon& B) { return A.GetStats().Slot < B.GetStats().Slot; });

	const int32 CurrentIndex = FMath::Max(0, Sorted.IndexOfByKey(CurrentWeapon.Get()));
	const int32 NextIndex = (CurrentIndex + (Direction >= 0 ? 1 : -1) + Sorted.Num()) % Sorted.Num();
	EquipWeapon(Sorted[NextIndex]);
}

void ANACharacterBase::OnRep_Weapons()
{
	for (ANAWeapon* Weapon : Weapons)
	{
		AttachWeapon(Weapon);
	}
}

void ANACharacterBase::OnRep_CurrentWeapon()
{
	if (CurrentWeapon)
	{
		AttachWeapon(CurrentWeapon);
	}
	UpdateRotationMode();
}

void ANACharacterBase::AttachWeapon(ANAWeapon* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	static const FName HandSocket(TEXT("hand_r"));
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (HasRealBody() && MeshComp->DoesSocketExist(HandSocket))
	{
		Weapon->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocket);
	}
	else
	{
		// Stand-in body: hold the gun at the right side, chest height, pointing forward.
		Weapon->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->SetActorRelativeLocation(FVector(30.f, 24.f, 25.f));
	}
}

void ANACharacterBase::SetAiming(bool bNewAiming)
{
	if (IsDead() || !CurrentWeapon)
	{
		bNewAiming = false;
	}
	if (bIsAiming == bNewAiming)
	{
		return;
	}

	bIsAiming = bNewAiming;
	if (bIsAiming && Gait == ENAGait::Sprint)
	{
		SetDesiredGait(ENAGait::Run);
	}
	OnRep_Aiming();

	if (!HasAuthority())
	{
		ServerSetAiming(bNewAiming);
	}
}

void ANACharacterBase::ServerSetAiming_Implementation(bool bNewAiming)
{
	SetAiming(bNewAiming);
}

void ANACharacterBase::OnRep_Aiming()
{
	ApplyGait();
	UpdateRotationMode();
}

void ANACharacterBase::UpdateRotationMode()
{
	// While aiming, the body faces where the camera looks (strafe); otherwise it faces its movement.
	const bool bStrafe = bIsAiming && CurrentWeapon != nullptr;
	GetCharacterMovement()->bOrientRotationToMovement = !bStrafe;
	GetCharacterMovement()->bUseControllerDesiredRotation = bStrafe;
}

void ANACharacterBase::GetAimViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	GetActorEyesViewPoint(OutLocation, OutRotation);
}

void ANACharacterBase::NotifyHitConfirmed(float Damage, bool bHeadshot, bool bKilled)
{
	LastHitConfirmTime = GetWorld()->GetTimeSeconds();
	LastHitDamage = Damage;
	bLastHitHeadshot = bHeadshot;
	bLastHitKill = bKilled;
}

// ----- Appearance ------------------------------------------------------------------------------

bool ANACharacterBase::HasRealBody() const
{
	return GetMesh() && GetMesh()->GetSkeletalMeshAsset() != nullptr;
}

void ANACharacterBase::UpdateFallbackBodyHeight()
{
	const float HalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float Radius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();

	FallbackBody->SetRelativeScale3D(FVector(
		(Radius * 2.f - 10.f) / NACharacter::ShapeSize,
		(Radius * 2.f - 10.f) / NACharacter::ShapeSize,
		(HalfHeight * 2.f) / NACharacter::ShapeSize));

	FallbackVisor->SetRelativeLocation(FVector(Radius - 8.f, 0.f, HalfHeight - 25.f));
}

void ANACharacterBase::ResolveAppearance()
{
	USkeletalMeshComponent* MeshComp = GetMesh();

	// Only touch disk if the asset is actually in the project; a missing pack is expected, not an error.
	auto AssetExists = [](const FSoftObjectPath& Path)
	{
		return Path.IsValid() && FPackageName::DoesPackageExist(Path.GetLongPackageName());
	};

	if (!MeshComp->GetSkeletalMeshAsset() && AssetExists(BodyMesh.ToSoftObjectPath()))
	{
		if (USkeletalMesh* Loaded = BodyMesh.LoadSynchronous())
		{
			MeshComp->SetSkeletalMeshAsset(Loaded);
		}
	}

	if (MeshComp->GetSkeletalMeshAsset() && !MeshComp->GetAnimInstance() && AssetExists(BodyAnimClass.ToSoftObjectPath()))
	{
		if (UClass* AnimClass = BodyAnimClass.LoadSynchronous())
		{
			MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			MeshComp->SetAnimInstanceClass(AnimClass);
		}
	}

	const bool bRealBody = HasRealBody();
	FallbackBody->SetVisibility(!bRealBody);
	FallbackVisor->SetVisibility(!bRealBody);

	// With a real skeleton, bullets hit the mesh (per-bone headshots); with a stand-in they hit the capsule.
	GetCapsuleComponent()->SetCollisionResponseToChannel(NA_TRACE_WEAPON, bRealBody ? ECR_Ignore : ECR_Block);
	MeshComp->SetCollisionResponseToChannel(NA_TRACE_WEAPON, bRealBody ? ECR_Block : ECR_Ignore);

	if (!bRealBody)
	{
		UE_LOG(LogNullAction, Verbose,
			TEXT("%s: no skeletal mesh found at %s. Showing stand-in body. Add the Third Person content pack or set BodyMesh in a Blueprint subclass."),
			*GetName(), *BodyMesh.ToString());
	}

	// Weapons attached before the body loaded may need to move to the hand socket.
	for (ANAWeapon* Weapon : Weapons)
	{
		AttachWeapon(Weapon);
	}
}
