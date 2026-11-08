#include "Weapons/NAWeapon.h"

#include "NullAction.h"
#include "Character/NACharacterBase.h"
#include "Combat/NACombatTypes.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/NATracer.h"

namespace NAWeaponConstants
{
	constexpr float CubeSize = 100.f;
	/** How far the client's reported view origin may be from its character before the server ignores it. */
	constexpr float MaxViewOffsetCm = 600.f;
}

ANAWeapon::ANAWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false); // Attachment to the owner is set up on every machine.

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakePart = [this](const TCHAR* PartName)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
		Part->SetupAttachment(Root);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(true);
		if (Cube.Succeeded())
		{
			Part->SetStaticMesh(Cube.Object);
		}
		return Part;
	};
	BodyMesh = MakePart(TEXT("BodyMesh"));
	GripMesh = MakePart(TEXT("GripMesh"));

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(Root);

	MuzzleFlash = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlash"));
	MuzzleFlash->SetupAttachment(Muzzle);
	MuzzleFlash->SetIntensity(20000.f);
	MuzzleFlash->SetAttenuationRadius(350.f);
	MuzzleFlash->SetLightColor(FLinearColor(1.f, 0.65f, 0.3f));
	MuzzleFlash->SetCastShadows(false);
	MuzzleFlash->SetVisibility(false);
}

void ANAWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ANAWeapon, AmmoInMagazine, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ANAWeapon, ReserveAmmo, COND_OwnerOnly);
	DOREPLIFETIME(ANAWeapon, bReloading);
}

void ANAWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Shape the stand-in model from this weapon's stats (subclass constructors have run by now).
	const FVector Size = Stats.ModelSize;
	BodyMesh->SetRelativeLocation(FVector(Size.X * 0.5f - 8.f, 0.f, 0.f));
	BodyMesh->SetRelativeScale3D(Size / NAWeaponConstants::CubeSize);
	GripMesh->SetRelativeLocation(FVector(0.f, 0.f, -Size.Z * 0.9f));
	GripMesh->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
	GripMesh->SetRelativeScale3D(FVector(5.f, Size.Y * 0.9f, 12.f) / NAWeaponConstants::CubeSize);
	Muzzle->SetRelativeLocation(FVector(Size.X - 6.f, 0.f, Size.Z * 0.15f));

	if (HasAuthority())
	{
		AmmoInMagazine = Stats.MagazineSize;
		ReserveAmmo = FMath::Clamp(Stats.StartingReserveAmmo, 0, Stats.MaxReserveAmmo);
	}
}

ANACharacterBase* ANAWeapon::GetOwningCharacter() const
{
	return Cast<ANACharacterBase>(GetOwner());
}

FVector ANAWeapon::GetMuzzleLocation() const
{
	return Muzzle->GetComponentLocation();
}

float ANAWeapon::GetFireInterval() const
{
	return 60.f / FMath::Max(Stats.RoundsPerMinute, 1.f);
}

float ANAWeapon::GetCurrentSpreadDegrees() const
{
	const ANACharacterBase* Character = GetOwningCharacter();
	if (!Character)
	{
		return Stats.HipSpreadDegrees;
	}

	float Spread = Character->IsAiming() ? Stats.AimSpreadDegrees : Stats.HipSpreadDegrees;
	const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if (Move && Move->IsFalling())
	{
		Spread *= 2.5f;
	}
	else if (Character->GetVelocity().Size2D() > 50.f)
	{
		Spread *= Character->IsSprinting() ? 2.f : 1.4f;
	}
	if (Character->bIsCrouched)
	{
		Spread *= 0.7f;
	}
	return Spread;
}

void ANAWeapon::StartFire()
{
	FireOnce();
	if (Stats.FireMode == ENAFireMode::FullAuto)
	{
		GetWorldTimerManager().SetTimer(AutoFireTimer, this, &ANAWeapon::FireOnce, GetFireInterval(), true);
	}
}

void ANAWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(AutoFireTimer);
}

void ANAWeapon::FireOnce()
{
	ANACharacterBase* Character = GetOwningCharacter();
	if (!Character || Character->IsDead() || bReloading || IsHidden())
	{
		StopFire();
		return;
	}

	if (AmmoInMagazine <= 0)
	{
		StopFire();
		RequestReload();
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastLocalFireTime < GetFireInterval() * 0.95f)
	{
		return; // Too soon (e.g. clicking faster than the gun can fire).
	}
	LastLocalFireTime = Now;

	FVector ViewLocation;
	FRotator ViewRotation;
	Character->GetAimViewPoint(ViewLocation, ViewRotation);

	// Recoil is applied immediately on the shooter's machine so the gun feels responsive.
	Character->ApplyRecoil(Stats.RecoilPitchDegrees, Stats.RecoilYawDegrees);

	if (HasAuthority())
	{
		ExecuteShotOnServer(ViewLocation, ViewRotation.Vector());
	}
	else
	{
		ServerFire(ViewLocation, ViewRotation.Vector());
	}
}

void ANAWeapon::ServerFire_Implementation(FVector_NetQuantize ViewLocation, FVector_NetQuantizeNormal ViewDirection)
{
	ExecuteShotOnServer(ViewLocation, ViewDirection);
}

void ANAWeapon::ExecuteShotOnServer(const FVector& ViewLocation, const FVector& ViewDirection)
{
	ANACharacterBase* Character = GetOwningCharacter();
	UWorld* World = GetWorld();
	if (!Character || !World || Character->IsDead() || bReloading || IsHidden() || AmmoInMagazine <= 0)
	{
		return;
	}

	// Anti-cheat: enforce fire rate (with a little slack for network jitter).
	const float Now = World->GetTimeSeconds();
	if (Now - LastServerFireTime < GetFireInterval() * 0.8f)
	{
		return;
	}
	LastServerFireTime = Now;

	const FVector Direction = ViewDirection.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return;
	}

	// Anti-cheat: a client may not claim to shoot from somewhere far from its body.
	FVector Origin = ViewLocation;
	if (FVector::DistSquared(Origin, Character->GetActorLocation()) > FMath::Square(NAWeaponConstants::MaxViewOffsetCm))
	{
		FRotator Unused;
		Character->GetAimViewPoint(Origin, Unused);
	}

	--AmmoInMagazine;

	const FVector Eye = Character->GetPawnViewLocation();
	// Start the camera ray level with the character, so objects between camera and body can't be hit.
	Origin += Direction * FMath::Max(0.f, FVector::DotProduct(Eye - Origin, Direction));

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NAWeaponFire), /*bTraceComplex*/ false, this);
	Params.AddIgnoredActor(Character);

	const float HalfConeRadians = FMath::DegreesToRadians(GetCurrentSpreadDegrees() * 0.5f);
	const int32 Pellets = FMath::Max(1, Stats.PelletsPerShot);

	TArray<FVector_NetQuantize> Ends;
	Ends.Reserve(Pellets);
	float TotalDamage = 0.f;
	bool bAnyHeadshot = false;
	bool bAnyKill = false;

	for (int32 Pellet = 0; Pellet < Pellets; ++Pellet)
	{
		const FVector ShotDirection = FMath::VRandCone(Direction, HalfConeRadians);
		const FVector RayEnd = Origin + ShotDirection * Stats.RangeCm;

		FVector Target = RayEnd;
		FHitResult ViewHit;
		if (World->LineTraceSingleByChannel(ViewHit, Origin, RayEnd, NA_TRACE_WEAPON, Params))
		{
			Target = ViewHit.ImpactPoint;
		}

		// Second trace from the character's eyes: you can only hit what your body can actually see.
		const FVector ToTarget = (Target - Eye).GetSafeNormal();
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Eye, Target + ToTarget * 10.f, NA_TRACE_WEAPON, Params);
		Ends.Add(bHit ? FVector(Hit.ImpactPoint) : Target);

		if (!bHit || !Hit.GetActor())
		{
			continue;
		}

		const ENAHitZone Zone = NACombat::GetHitZone(Hit);
		const float Multiplier = Zone == ENAHitZone::Head ? Stats.HeadshotMultiplier
			: Zone == ENAHitZone::Limb ? Stats.LimbMultiplier : 1.f;

		const float Dealt = UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Stats.Damage * Multiplier, ToTarget, Hit,
			Character->GetController(), this, UDamageType::StaticClass());

		if (Dealt > 0.f)
		{
			TotalDamage += Dealt;
			bAnyHeadshot |= Zone == ENAHitZone::Head;
			if (const ANACharacterBase* Victim = Cast<ANACharacterBase>(Hit.GetActor()))
			{
				bAnyKill |= Victim->IsDead();
			}
		}
	}

	MulticastShotEffects(GetMuzzleLocation(), Ends);

	// Enemies within hearing range notice gunfire.
	UAISense_Hearing::ReportNoiseEvent(World, Eye, 1.f, Character, 0.f, FName(TEXT("Gunshot")));

	if (TotalDamage > 0.f)
	{
		ClientHitConfirmed(TotalDamage, bAnyHeadshot, bAnyKill);
	}

	if (AmmoInMagazine <= 0)
	{
		StartReloadOnServer(); // Auto-reload on empty.
	}
}

void ANAWeapon::MulticastShotEffects_Implementation(FVector_NetQuantize Start, const TArray<FVector_NetQuantize>& Ends)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return; // Nothing to see on a server with no screen.
	}

	MuzzleFlash->SetVisibility(true);
	GetWorldTimerManager().SetTimer(FlashTimer, this, &ANAWeapon::HideMuzzleFlash, 0.05f, false);

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Start);
	}

	for (const FVector_NetQuantize& End : Ends)
	{
		ANATracer::Spawn(GetWorld(), Start, End);
	}
}

void ANAWeapon::HideMuzzleFlash()
{
	MuzzleFlash->SetVisibility(false);
}

void ANAWeapon::ClientHitConfirmed_Implementation(float Damage, bool bHeadshot, bool bKilled)
{
	if (ANACharacterBase* Character = GetOwningCharacter())
	{
		Character->NotifyHitConfirmed(Damage, bHeadshot, bKilled);
	}
}

void ANAWeapon::RequestReload()
{
	if (bReloading || AmmoInMagazine >= Stats.MagazineSize || ReserveAmmo <= 0)
	{
		return;
	}

	if (HasAuthority())
	{
		StartReloadOnServer();
	}
	else
	{
		ServerReload();
	}
}

void ANAWeapon::ServerReload_Implementation()
{
	StartReloadOnServer();
}

void ANAWeapon::StartReloadOnServer()
{
	if (!HasAuthority() || bReloading || AmmoInMagazine >= Stats.MagazineSize || ReserveAmmo <= 0)
	{
		return;
	}

	StopFire();
	bReloading = true;
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &ANAWeapon::FinishReload, Stats.ReloadSeconds, false);

	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}
}

void ANAWeapon::FinishReload()
{
	const int32 Needed = Stats.MagazineSize - AmmoInMagazine;
	const int32 Moved = FMath::Min(Needed, ReserveAmmo);
	AmmoInMagazine += Moved;
	ReserveAmmo -= Moved;
	bReloading = false;
}

void ANAWeapon::OnEquipped()
{
	SetActorHiddenInGame(false);
}

void ANAWeapon::OnUnequipped()
{
	StopFire();
	if (bReloading)
	{
		GetWorldTimerManager().ClearTimer(ReloadTimer);
		bReloading = false; // Switching weapons cancels a reload, like most shooters.
	}
	SetActorHiddenInGame(true);
}

int32 ANAWeapon::AddReserveAmmo(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0)
	{
		return 0;
	}
	const int32 Old = ReserveAmmo;
	ReserveAmmo = FMath::Min(ReserveAmmo + Amount, Stats.MaxReserveAmmo);
	return ReserveAmmo - Old;
}
