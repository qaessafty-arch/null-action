#include "Creatures/NABoss.h"

#include "NullAction.h"
#include "AIController.h"
#include "Character/NACharacterBase.h"
#include "Character/NAPlayerCharacter.h"
#include "Combat/NACombatTypes.h"
#include "Combat/NAHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAGameMode.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/GameStateBase.h"
#include "Interaction/NAResourceNode.h"
#include "Inventory/NAItems.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Core/NAAssetSettings.h"

namespace NABossTuning
{
	constexpr float ShapeSize = 100.f;
	constexpr float BaseHealth = 1800.f;
	constexpr float HealthPerExtraPlayer = 900.f;
	constexpr float WakeRadius = 2200.f;
	constexpr float LeashRadius = 6000.f;
	constexpr float SlamRadius = 650.f;
	constexpr float SlamDamage = 45.f;
	constexpr float RainRadius = 380.f;
	constexpr float RainDamage = 35.f;
	constexpr float ChargeDamage = 40.f;
	constexpr float ChargeSpeed = 1150.f;
	constexpr int32 MaxDiscs = 4;
	const FVector HipPositions[4] = { FVector(130.f, -95.f, -40.f), FVector(130.f, 95.f, -40.f), FVector(-130.f, -95.f, -40.f), FVector(-130.f, 95.f, -40.f) };
}

ANABoss::ANABoss(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true; // Everyone should see (and hear about) the boss.
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(150.f, 190.f);
	Capsule->SetCollisionResponseToChannel(NA_TRACE_WEAPON, ECR_Ignore); // Bullets hit the body parts.

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 120.f, 0.f); // Turns slowly: get behind it.
	Move->MaxWalkSpeed = 350.f;
	Move->MaxAcceleration = 1500.f;
	Move->MaxStepHeight = 90.f;
	Move->SetWalkableFloorAngle(50.f);

	GetMesh()->SetVisibility(false);

	HealthComponent = CreateDefaultSubobject<UNAHealthComponent>(TEXT("Health"));
	HealthComponent->SetMaxHealth(NABossTuning::BaseHealth);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	UStaticMesh* CubeShape = Cube.Object;
	UStaticMesh* SphereShape = Sphere.Object;
	UStaticMesh* CylinderShape = Cylinder.Object;
	UStaticMesh* ConeShape = Cone.Object;

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(Capsule);
	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, 10.f));

	// Body: a hunched, armored mass. ~5 m long, ~4 m tall.
	MakePart(TEXT("Torso"), VisualRoot, CubeShape, FVector(0.f, 0.f, 40.f), FVector(360.f, 230.f, 170.f), FRotator(-6.f, 0.f, 0.f), false, TEXT("NA.Body"));
	MakePart(TEXT("Shoulders"), VisualRoot, SphereShape, FVector(110.f, 0.f, 90.f), FVector(240.f, 260.f, 200.f), FRotator::ZeroRotator, false, TEXT("NA.Body"));
	MakePart(TEXT("Haunch"), VisualRoot, SphereShape, FVector(-140.f, 0.f, 50.f), FVector(200.f, 220.f, 160.f), FRotator::ZeroRotator, false, TEXT("NA.Body"));
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const float X = 120.f - Index * 60.f;
		MakePart(*FString::Printf(TEXT("Plate_%d"), Index), VisualRoot, CubeShape, FVector(X, 0.f, 150.f - Index * 6.f),
			FVector(70.f, 200.f - Index * 15.f, 30.f), FRotator(-10.f, 0.f, 0.f), false, TEXT("NA.Body"));
		MakePart(*FString::Printf(TEXT("Spike_%d"), Index), VisualRoot, ConeShape, FVector(X, 0.f, 200.f - Index * 6.f),
			FVector(40.f, 40.f, 90.f - Index * 8.f), FRotator(-15.f, 0.f, 0.f), true);
	}
	// Weak point: the exposed ember core between the back plates.
	MakePart(TEXT("Core"), VisualRoot, SphereShape, FVector(-20.f, 0.f, 170.f), FVector(80.f), FRotator::ZeroRotator, true, TEXT("NA.Head"));
	CoreGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("CoreGlow"));
	CoreGlow->SetupAttachment(VisualRoot);
	CoreGlow->SetRelativeLocation(FVector(-20.f, 0.f, 230.f));
	CoreGlow->SetLightColor(FLinearColor(1.f, 0.4f, 0.1f));
	CoreGlow->SetIntensity(9000.f);
	CoreGlow->SetAttenuationRadius(1200.f);
	CoreGlow->SetCastShadows(false);

	HeadJoint = CreateDefaultSubobject<USceneComponent>(TEXT("HeadJoint"));
	HeadJoint->SetupAttachment(VisualRoot);
	HeadJoint->SetRelativeLocation(FVector(230.f, 0.f, 60.f));
	MakePart(TEXT("Head"), HeadJoint, CubeShape, FVector(60.f, 0.f, 0.f), FVector(150.f, 130.f, 100.f), FRotator(-10.f, 0.f, 0.f), false, TEXT("NA.Head"));
	MakePart(TEXT("Jaw"), HeadJoint, CubeShape, FVector(80.f, 0.f, -55.f), FVector(130.f, 110.f, 30.f), FRotator(5.f, 0.f, 0.f), false, TEXT("NA.Head"));
	MakePart(TEXT("HornL"), HeadJoint, ConeShape, FVector(20.f, -70.f, 60.f), FVector(40.f, 40.f, 120.f), FRotator(-40.f, 0.f, -30.f), false);
	MakePart(TEXT("HornR"), HeadJoint, ConeShape, FVector(20.f, 70.f, 60.f), FVector(40.f, 40.f, 120.f), FRotator(-40.f, 0.f, 30.f), false);
	MakePart(TEXT("EyeL"), HeadJoint, SphereShape, FVector(130.f, -40.f, 20.f), FVector(20.f), FRotator::ZeroRotator, true);
	MakePart(TEXT("EyeR"), HeadJoint, SphereShape, FVector(130.f, 40.f, 20.f), FVector(20.f), FRotator::ZeroRotator, true);

	// Four pillar legs.
	for (int32 Leg = 0; Leg < 4; ++Leg)
	{
		USceneComponent* Hip = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("Hip_%d"), Leg));
		Hip->SetupAttachment(VisualRoot);
		Hip->SetRelativeLocation(NABossTuning::HipPositions[Leg]);
		Hips.Add(Hip);
		MakePart(*FString::Printf(TEXT("Leg_%d"), Leg), Hip, CylinderShape, FVector(0.f, 0.f, -80.f), FVector(80.f, 80.f, 170.f), FRotator::ZeroRotator, false, TEXT("NA.Limb"));
		MakePart(*FString::Printf(TEXT("Foot_%d"), Leg), Hip, CubeShape, FVector(15.f, 0.f, -165.f), FVector(110.f, 95.f, 30.f), FRotator::ZeroRotator, false, TEXT("NA.Limb"));
	}

	// Telegraph circles: flat red discs, placed in world space.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	for (int32 Index = 0; Index < NABossTuning::MaxDiscs; ++Index)
	{
		UStaticMeshComponent* Disc = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("WarningDisc_%d"), Index));
		Disc->SetupAttachment(Capsule);
		Disc->SetUsingAbsoluteLocation(true);
		Disc->SetUsingAbsoluteRotation(true);
		Disc->SetUsingAbsoluteScale(true);
		Disc->SetStaticMesh(CylinderShape);
		if (ShapeMaterial.Succeeded()) { Disc->SetMaterial(0, ShapeMaterial.Object); }
		Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Disc->CastShadow = false;
		Disc->SetVisibility(false);
		WarningDiscs.Add(Disc);
	}
}

UStaticMeshComponent* ANABoss::MakePart(FName PartName, USceneComponent* Parent, UStaticMesh* Shape, const FVector& Location,
	const FVector& SizeCm, const FRotator& Rotation, bool bEmber, FName HitTag)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
	Part->SetupAttachment(Parent);
	Part->SetStaticMesh(Shape);
	Part->SetRelativeLocationAndRotation(Location, Rotation);
	Part->SetRelativeScale3D(SizeCm / NABossTuning::ShapeSize);
	Part->SetGenerateOverlapEvents(false);
	if (ShapeMaterial.Succeeded()) { Part->SetMaterial(0, ShapeMaterial.Object); }
	if (HitTag != NAME_None)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Part->SetCollisionResponseToAllChannels(ECR_Ignore);
		Part->SetCollisionResponseToChannel(NA_TRACE_WEAPON, ECR_Block);
		Part->ComponentTags.Add(HitTag);
		HitParts.Add(Part);
	}
	else
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	(bEmber ? EmberParts : AshParts).Add(Part);
	return Part;
}

void ANABoss::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANABoss, BossState);
	DOREPLIFETIME(ANABoss, Phase);
	DOREPLIFETIME(ANABoss, WarningLocations);
	DOREPLIFETIME(ANABoss, WarningRadii);
	DOREPLIFETIME(ANABoss, WarningEnds);
	DOREPLIFETIME(ANABoss, SlamCount);
}

void ANABoss::BeginPlay()
{
	Super::BeginPlay();
	if (LairLocation.IsZero())
	{
		LairLocation = GetActorLocation();
	}
	for (UStaticMeshComponent* Part : AshParts)
	{
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.07f, 0.06f, 0.055f));
		}
	}
	for (UStaticMeshComponent* Part : EmberParts)
	{
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.32f, 0.03f));
		}
	}
	for (UStaticMeshComponent* Disc : WarningDiscs)
	{
		if (UMaterialInstanceDynamic* MID = Disc->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.9f, 0.05f, 0.02f));
		}
	}
	if (NAAssets::ApplyModel(UNAAssetSettings::Get()->Boss, GetMesh(), GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()))
	{
		VisualRoot->SetVisibility(false, true); // Real model; invisible shape parts still take the hits.
	}
	HealthComponent->OnDeathStateChanged.AddDynamic(this, &ANABoss::HandleDeathStateChanged);
	if (HasAuthority())
	{
		HealthComponent->OnKilled.AddDynamic(this, &ANABoss::HandleKilled);
	}
	OnRep_BossState();
}

bool ANABoss::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

float ANABoss::ServerNow() const
{
	const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return State ? static_cast<float>(State->GetServerWorldTimeSeconds()) : 0.f;
}

float ANABoss::PhaseSpeed() const
{
	return Phase >= 3 ? 520.f : Phase == 2 ? 420.f : 350.f;
}

float ANABoss::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (BossState == ENABossState::Dead)
	{
		return 0.f;
	}
	if (BossState == ENABossState::Dormant)
	{
		Wake(EventInstigator ? EventInstigator->GetPawn() : nullptr);
	}
	float Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Damage <= 0.f || !HealthComponent)
	{
		return 0.f;
	}
	ENAHitZone Zone = ENAHitZone::Body;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		Zone = NACombat::GetHitZone(static_cast<const FPointDamageEvent&>(DamageEvent).HitInfo);
	}
	// Armored hide: aim for the ember core and the head.
	if (Zone == ENAHitZone::Body)      { Damage *= 0.5f; }
	else if (Zone == ENAHitZone::Limb) { Damage *= 0.7f; }
	return HealthComponent->ApplyDamage(Damage, Zone, EventInstigator, DamageCauser);
}

APawn* ANABoss::FindTarget(float Radius) const
{
	APawn* Best = nullptr;
	float BestDistance = Radius;
	for (TActorIterator<ANAPlayerCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsDead() || It->IsDowned())
		{
			continue;
		}
		const float Distance = FVector::Dist(It->GetActorLocation(), GetActorLocation());
		if (Distance < BestDistance)
		{
			Best = *It;
			BestDistance = Distance;
		}
	}
	return Best;
}

void ANABoss::Wake(AActor* WokenBy)
{
	if (BossState != ENABossState::Dormant)
	{
		return;
	}
	// Scale health with the number of living players: nearly impossible solo unprepared, fair in a group.
	int32 Players = 0;
	for (TActorIterator<ANAPlayerCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead()) { ++Players; }
	}
	HealthComponent->SetMaxHealth(NABossTuning::BaseHealth + NABossTuning::HealthPerExtraPlayer * FMath::Max(0, Players - 1));
	BossState = ENABossState::Awake;
	Phase = 1;
	CurrentTarget = Cast<APawn>(WokenBy);
	LastTargetSeenTime = ServerNow();
	NextAttackTime = ServerNow() + 2.f;
	BusyUntil = ServerNow() + 2.f; // Roar.
	OnRep_BossState();
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->AnnounceToAll(TEXT("The ASHEN BEHEMOTH has awoken in the Ash Crater!"));
	}
}

void ANABoss::ResetToLair()
{
	BossState = ENABossState::Dormant;
	Phase = 1;
	CurrentTarget = nullptr;
	PendingStrikes.Reset();
	WarningLocations.Reset();
	WarningRadii.Reset();
	WarningEnds.Reset();
	HealthComponent->SetMaxHealth(NABossTuning::BaseHealth);
	TeleportTo(LairLocation, GetActorRotation(), false, true);
	OnRep_BossState();
}

void ANABoss::AddWarning(const FVector& Location, float Radius, float Delay, float Damage)
{
	const float When = ServerNow() + Delay;
	PendingStrikes.Add({ Location, Radius, When, Damage });
	WarningLocations.Add(Location);
	WarningRadii.Add(Radius);
	WarningEnds.Add(When);
}

void ANABoss::StartSlam()
{
	const float Delay = Phase >= 3 ? 0.8f : 1.2f;
	AddWarning(GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), NABossTuning::SlamRadius, Delay,
		NABossTuning::SlamDamage * (Phase >= 3 ? 1.25f : 1.f));
	BusyUntil = ServerNow() + Delay + 0.6f;
	GetCharacterMovement()->StopMovementImmediately();
}

void ANABoss::StartEmberRain(const APawn* Victim)
{
	if (!Victim)
	{
		return;
	}
	const float Delay = Phase >= 3 ? 1.0f : 1.4f;
	const FVector Center = Victim->GetActorLocation() - FVector(0.f, 0.f, 90.f);
	AddWarning(Center, NABossTuning::RainRadius, Delay, NABossTuning::RainDamage);
	AddWarning(Center + FVector(FMath::FRandRange(-500.f, 500.f), FMath::FRandRange(-500.f, 500.f), 0.f), NABossTuning::RainRadius, Delay + 0.4f, NABossTuning::RainDamage);
	AddWarning(Center + Victim->GetVelocity().GetSafeNormal2D() * 450.f, NABossTuning::RainRadius, Delay + 0.8f, NABossTuning::RainDamage); // Where you're running.
	BusyUntil = ServerNow() + 1.2f;
}

void ANABoss::StartCharge(const APawn* Victim)
{
	if (!Victim)
	{
		return;
	}
	ChargeDirection = (Victim->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	ChargeUntil = ServerNow() + 2.f;
	bChargeHit = false;
	GetCharacterMovement()->MaxWalkSpeed = NABossTuning::ChargeSpeed;
}

void ANABoss::ResolveWarnings()
{
	const float Now = ServerNow();
	for (int32 Index = PendingStrikes.Num() - 1; Index >= 0; --Index)
	{
		const FPendingStrike Strike = PendingStrikes[Index];
		if (Now < Strike.Time)
		{
			continue;
		}
		TArray<AActor*> Ignored;
		Ignored.Add(this);
		UGameplayStatics::ApplyRadialDamage(this, Strike.Damage, Strike.Location + FVector(0.f, 0.f, 60.f), Strike.Radius, UDamageType::StaticClass(),
			Ignored, this, GetController(), /*bDoFullDamage*/ true, ECC_Visibility);
		// The shockwave throws players.
		for (TActorIterator<ANAPlayerCharacter> It(GetWorld()); It; ++It)
		{
			const FVector Away = It->GetActorLocation() - Strike.Location;
			if (!It->IsDead() && Away.Size2D() <= Strike.Radius && FMath::Abs(Away.Z) < 400.f)
			{
				It->LaunchCharacter(Away.GetSafeNormal2D() * 700.f + FVector(0.f, 0.f, 450.f), true, true);
			}
		}
		++SlamCount;
		OnRep_SlamCount();
		PendingStrikes.RemoveAt(Index);
		if (WarningLocations.IsValidIndex(Index))
		{
			WarningLocations.RemoveAt(Index);
			WarningRadii.RemoveAt(Index);
			WarningEnds.RemoveAt(Index);
		}
	}
}

void ANABoss::ThinkServer(float DeltaSeconds)
{
	const float Now = ServerNow();
	ResolveWarnings();

	if (BossState == ENABossState::Dormant)
	{
		if (APawn* Intruder = FindTarget(NABossTuning::WakeRadius))
		{
			Wake(Intruder);
		}
		return;
	}
	if (BossState != ENABossState::Awake)
	{
		return;
	}

	// Phase changes.
	const float HealthFraction = HealthComponent->GetHealth() / FMath::Max(1.f, HealthComponent->GetMaxHealth());
	const uint8 NewPhase = HealthFraction < 0.25f ? 3 : HealthFraction < 0.6f ? 2 : 1;
	if (NewPhase > Phase)
	{
		Phase = NewPhase;
		if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
		{
			GameMode->AnnounceToAll(Phase >= 3 ? TEXT("The Ashen Behemoth is ENRAGED!") : TEXT("The Ashen Behemoth's core flares - embers rain down!"));
		}
	}

	// Pick or keep a target inside the leash.
	APawn* Victim = CurrentTarget.Get();
	const ANACharacterBase* VictimBody = Cast<ANACharacterBase>(Victim);
	if (!VictimBody || VictimBody->IsDead() || VictimBody->IsDowned() || FVector::Dist(Victim->GetActorLocation(), LairLocation) > NABossTuning::LeashRadius)
	{
		Victim = FindTarget(NABossTuning::LeashRadius);
		CurrentTarget = Victim;
	}
	if (Victim)
	{
		LastTargetSeenTime = Now;
	}
	if (FVector::Dist(GetActorLocation(), LairLocation) > NABossTuning::LeashRadius + 1000.f || Now - LastTargetSeenTime > 10.f)
	{
		ResetToLair(); // Everyone fled or fell: go home and heal.
		return;
	}
	if (!Victim)
	{
		return;
	}

	// Charging: barrel forward, hit the first player in the way.
	if (Now < ChargeUntil)
	{
		AddMovementInput(ChargeDirection, 1.f);
		if (!bChargeHit)
		{
			for (TActorIterator<ANAPlayerCharacter> It(GetWorld()); It; ++It)
			{
				if (!It->IsDead() && FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) < 260.f)
				{
					bChargeHit = true;
					UGameplayStatics::ApplyDamage(*It, NABossTuning::ChargeDamage, GetController(), this, UDamageType::StaticClass());
					It->LaunchCharacter(ChargeDirection * 900.f + FVector(0.f, 0.f, 350.f), true, true);
					break;
				}
			}
		}
		return;
	}
	GetCharacterMovement()->MaxWalkSpeed = PhaseSpeed();

	if (Now < BusyUntil)
	{
		return; // Mid-attack.
	}

	const float Distance = FVector::Dist2D(Victim->GetActorLocation(), GetActorLocation());
	if (Now >= NextAttackTime)
	{
		const float Gap = Phase >= 3 ? 2.2f : Phase == 2 ? 3.f : 3.6f;
		if (Distance < NABossTuning::SlamRadius * 0.9f)
		{
			StartSlam();
			NextAttackTime = Now + Gap;
			return;
		}
		if (Phase >= 2 && FMath::FRand() < 0.5f && Distance < 3500.f)
		{
			StartEmberRain(Victim);
			NextAttackTime = Now + Gap;
			return;
		}
		if (Distance > 900.f && Distance < 3000.f)
		{
			StartCharge(Victim);
			NextAttackTime = Now + Gap + 1.f;
			return;
		}
	}

	// Lumber toward the target.
	if (Distance > 350.f)
	{
		AddMovementInput((Victim->GetActorLocation() - GetActorLocation()).GetSafeNormal2D(), 1.f);
	}
}

void ANABoss::HandleKilled(AActor* Victim, AController* Killer)
{
	BossState = ENABossState::Dead;
	PendingStrikes.Reset();
	WarningLocations.Reset();
	WarningRadii.Reset();
	WarningEnds.Reset();
	OnRep_BossState();

	// Unique reward, spread around the carcass so a whole group can loot.
	const FVector Center = GetActorLocation();
	auto DropAt = [this, &Center](float Angle, FName Item, int32 Count)
	{
		const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * 320.f, FMath::Sin(FMath::DegreesToRadians(Angle)) * 320.f, 0.f);
		ANAItemPickup::Drop(GetWorld(), Center + Offset, Item, Count);
	};
	DropAt(0.f, TEXT("BehemothHeart"), 1);
	DropAt(60.f, NAItems::EmberFang(), 3);
	DropAt(120.f, NAItems::AshHide(), 4);
	DropAt(180.f, TEXT("AmmoPack"), 2);
	DropAt(240.f, NAItems::RawMeat(), 6);
	DropAt(300.f, NAItems::Scrap(), 8);

	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->OnBossKilled(this, Killer);
	}
}

void ANABoss::HandleDeathStateChanged(bool bNowDead)
{
	if (bNowDead)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
		for (UStaticMeshComponent* Part : HitParts)
		{
			Part->SetCollisionResponseToChannel(NA_TRACE_WEAPON, ECR_Ignore);
		}
		CoreGlow->SetVisibility(false);
	}
}

void ANABoss::OnRep_BossState()
{
	// Sleeping: dim core. Awake: blazing.
	if (CoreGlow)
	{
		CoreGlow->SetIntensity(BossState == ENABossState::Awake ? 14000.f : BossState == ENABossState::Dormant ? 2500.f : 0.f);
	}
}

void ANABoss::OnRep_SlamCount()
{
	SlamAnimStart = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

void ANABoss::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && BossState != ENABossState::Dead)
	{
		ThinkServer(DeltaSeconds);
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		AnimateBody(DeltaSeconds);
		UpdateWarningDiscs();
	}
}

void ANABoss::UpdateWarningDiscs()
{
	const float Now = ServerNow();
	for (int32 Index = 0; Index < WarningDiscs.Num(); ++Index)
	{
		UStaticMeshComponent* Disc = WarningDiscs[Index];
		const bool bActive = Index < WarningLocations.Num() && WarningRadii.IsValidIndex(Index) && WarningEnds.IsValidIndex(Index) && Now < WarningEnds[Index];
		Disc->SetVisibility(bActive);
		if (bActive)
		{
			// The circle pulses faster as the hit approaches.
			const float Left = WarningEnds[Index] - Now;
			const float Pulse = 0.92f + 0.08f * FMath::Sin(Now * (10.f + 20.f / FMath::Max(Left, 0.1f)));
			const float Diameter = WarningRadii[Index] * 2.f / NABossTuning::ShapeSize * Pulse;
			Disc->SetWorldLocation(FVector(WarningLocations[Index]) + FVector(0.f, 0.f, 4.f));
			Disc->SetWorldRotation(FRotator::ZeroRotator);
			Disc->SetWorldScale3D(FVector(Diameter, Diameter, 0.03f));
		}
	}
}

void ANABoss::AnimateBody(float DeltaSeconds)
{
	if (IsDead())
	{
		// Collapse onto its side.
		VisualRoot->SetRelativeRotation(FMath::RInterpTo(VisualRoot->GetRelativeRotation(), FRotator(0.f, 0.f, 70.f), DeltaSeconds, 1.5f));
		VisualRoot->SetRelativeLocation(FMath::VInterpTo(VisualRoot->GetRelativeLocation(), FVector(0.f, 0.f, -110.f), DeltaSeconds, 1.5f));
		return;
	}
	const float Speed = GetVelocity().Size2D();
	GaitPhase += DeltaSeconds * (1.2f + Speed / 220.f);
	for (int32 Leg = 0; Leg < Hips.Num(); ++Leg)
	{
		const bool bPairA = Leg == 0 || Leg == 3;
		const float Swing = FMath::Sin(GaitPhase + (bPairA ? 0.f : PI)) * FMath::Clamp(Speed / 400.f, 0.f, 1.f) * 22.f;
		Hips[Leg]->SetRelativeRotation(FRotator(Swing, 0.f, 0.f));
	}
	// Breathing, and a rear-up-and-slam when a strike lands.
	const float Now = GetWorld()->GetTimeSeconds();
	const float SinceSlam = Now - SlamAnimStart;
	const float Slam = SinceSlam < 0.5f ? FMath::Sin(SinceSlam / 0.5f * PI) : 0.f;
	const float Breath = FMath::Sin(Now * (BossState == ENABossState::Dormant ? 0.8f : 1.8f)) * 4.f;
	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, 10.f + Breath - Slam * 30.f));
	VisualRoot->SetRelativeRotation(FRotator(-Slam * 8.f, 0.f, 0.f));
	HeadJoint->SetRelativeRotation(FRotator(BossState == ENABossState::Dormant ? -20.f : FMath::Sin(Now * 1.3f) * 6.f, 0.f, 0.f));
}
