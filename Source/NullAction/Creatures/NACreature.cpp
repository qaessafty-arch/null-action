#include "Creatures/NACreature.h"

#include "NullAction.h"
#include "Combat/NAHealthComponent.h"
#include "Combat/NACombatTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAGameMode.h"
#include "Creatures/NACreatureAIController.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Interaction/NAResourceNode.h"
#include "Inventory/NAItems.h"
#include "Inventory/NAInventoryComponent.h"
#include "Character/NAPlayerCharacter.h"
#include "Hunting/NACreatureTrack.h"

namespace NACreatureBody
{
	constexpr float ShapeSize = 100.f;
	constexpr float CapsuleRadius = 55.f;
	constexpr float CapsuleHalfHeight = 60.f;
	// Leg order: front-left, front-right, back-left, back-right. Diagonal pairs move together (trot).
	const FVector HipPositions[4] = { FVector(55.f, -28.f, -10.f), FVector(55.f, 28.f, -10.f), FVector(-55.f, -28.f, -10.f), FVector(-55.f, 28.f, -10.f) };
	const float LegPhaseOffsets[4] = { 0.f, PI, PI, 0.f };
}

ANACreature::ANACreature(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = ANACreatureAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(NACreatureBody::CapsuleRadius, NACreatureBody::CapsuleHalfHeight);
	Capsule->SetCollisionResponseToChannel(NA_TRACE_WEAPON, ECR_Ignore); // Bullets hit the body parts instead.

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 360.f, 0.f);
	Move->MaxWalkSpeed = 220.f;
	Move->MaxAcceleration = 2400.f;
	Move->BrakingDecelerationWalking = 2000.f;

	GetMesh()->SetVisibility(false); // No skeletal model yet; the body below is built from shapes.

	HealthComponent = CreateDefaultSubobject<UNAHealthComponent>(TEXT("Health"));
	HealthComponent->SetMaxHealth(220.f); // Tough: aim for the glowing eyes (head-shot multiplier).

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* CubeShape = Cube.Object;
	UStaticMesh* CylinderShape = Cylinder.Object;
	UStaticMesh* SphereShape = Sphere.Object;

	VisualRoot = MakeJoint(TEXT("VisualRoot"), Capsule, FVector::ZeroVector);

	// Torso: long and low, deeper at the chest.
	MakePart(TEXT("Torso"), VisualRoot, CubeShape, FVector(0.f, 0.f, 8.f), FVector(150.f, 56.f, 44.f), FRotator::ZeroRotator, false, TEXT("NA.Body"));
	MakePart(TEXT("Chest"), VisualRoot, SphereShape, FVector(55.f, 0.f, 12.f), FVector(66.f, 66.f, 58.f), FRotator::ZeroRotator, false, TEXT("NA.Body"));
	MakePart(TEXT("Haunch"), VisualRoot, SphereShape, FVector(-55.f, 0.f, 10.f), FVector(58.f, 60.f, 50.f), FRotator::ZeroRotator, false, TEXT("NA.Body"));

	// Spines along the back glow like embers.
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const float X = -60.f + Index * 24.f;
		MakePart(*FString::Printf(TEXT("Spine_%d"), Index), VisualRoot, CubeShape, FVector(X, 0.f, 36.f + FMath::Sin(Index * 0.6f) * 4.f),
			FVector(5.f, 4.f, 18.f + (Index % 2) * 8.f), FRotator(-35.f, 0.f, 0.f), true);
	}

	// Neck and head (head is the weak point).
	NeckJoint = MakeJoint(TEXT("NeckJoint"), VisualRoot, FVector(88.f, 0.f, 22.f));
	MakePart(TEXT("Neck"), NeckJoint, CylinderShape, FVector(10.f, 0.f, 0.f), FVector(26.f, 26.f, 30.f), FRotator(-60.f, 0.f, 0.f), false, TEXT("NA.Body"));
	MakePart(TEXT("Head"), NeckJoint, CubeShape, FVector(42.f, 0.f, 6.f), FVector(58.f, 32.f, 28.f), FRotator(-8.f, 0.f, 0.f), false, TEXT("NA.Head"));
	MakePart(TEXT("Brow"), NeckJoint, CubeShape, FVector(38.f, 0.f, 22.f), FVector(40.f, 38.f, 8.f), FRotator(-14.f, 0.f, 0.f), false, TEXT("NA.Head"));
	MakePart(TEXT("EyeL"), NeckJoint, SphereShape, FVector(58.f, -13.f, 14.f), FVector(8.f), FRotator::ZeroRotator, true);
	MakePart(TEXT("EyeR"), NeckJoint, SphereShape, FVector(58.f, 13.f, 14.f), FVector(8.f), FRotator::ZeroRotator, true);

	JawJoint = MakeJoint(TEXT("JawJoint"), NeckJoint, FVector(20.f, 0.f, -8.f));
	MakePart(TEXT("Jaw"), JawJoint, CubeShape, FVector(26.f, 0.f, -4.f), FVector(50.f, 26.f, 9.f), FRotator::ZeroRotator, false, TEXT("NA.Head"));
	MakePart(TEXT("Fangs"), JawJoint, CubeShape, FVector(48.f, 0.f, 3.f), FVector(4.f, 22.f, 8.f), FRotator::ZeroRotator, true);

	EyeGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("EyeGlow"));
	EyeGlow->SetupAttachment(NeckJoint);
	EyeGlow->SetRelativeLocation(FVector(70.f, 0.f, 14.f));
	EyeGlow->SetIntensity(3000.f);
	EyeGlow->SetAttenuationRadius(250.f);
	EyeGlow->SetLightColor(FLinearColor(1.f, 0.4f, 0.08f));
	EyeGlow->SetCastShadows(false);

	// Four two-segment legs.
	for (int32 Leg = 0; Leg < 4; ++Leg)
	{
		USceneComponent* Hip = MakeJoint(*FString::Printf(TEXT("Hip_%d"), Leg), VisualRoot, NACreatureBody::HipPositions[Leg]);
		MakePart(*FString::Printf(TEXT("Thigh_%d"), Leg), Hip, CylinderShape, FVector(0.f, 0.f, -12.f), FVector(15.f, 15.f, 26.f), FRotator::ZeroRotator, false, TEXT("NA.Limb"));
		USceneComponent* Knee = MakeJoint(*FString::Printf(TEXT("Knee_%d"), Leg), Hip, FVector(0.f, 0.f, -24.f));
		MakePart(*FString::Printf(TEXT("Shin_%d"), Leg), Knee, CylinderShape, FVector(0.f, 0.f, -13.f), FVector(10.f, 10.f, 26.f), FRotator::ZeroRotator, false, TEXT("NA.Limb"));
		MakePart(*FString::Printf(TEXT("Claw_%d"), Leg), Knee, CubeShape, FVector(6.f, 0.f, -26.f), FVector(16.f, 10.f, 4.f), FRotator::ZeroRotator, false);
		Hips.Add(Hip);
		Knees.Add(Knee);
	}

	// Tail: a chain of joints that sways.
	USceneComponent* TailParent = VisualRoot;
	FVector TailOffset(-78.f, 0.f, 14.f);
	for (int32 Segment = 0; Segment < 5; ++Segment)
	{
		USceneComponent* Joint = MakeJoint(*FString::Printf(TEXT("Tail_%d"), Segment), TailParent, TailOffset);
		const float Thickness = 16.f - Segment * 2.5f;
		MakePart(*FString::Printf(TEXT("TailSeg_%d"), Segment), Joint, CylinderShape, FVector(-14.f, 0.f, 0.f),
			FVector(Thickness, Thickness, 30.f), FRotator(90.f, 0.f, 0.f), Segment == 4);
		TailJoints.Add(Joint);
		TailParent = Joint;
		TailOffset = FVector(-28.f, 0.f, -2.f);
	}
}

USceneComponent* ANACreature::MakeJoint(FName JointName, USceneComponent* Parent, const FVector& Location)
{
	USceneComponent* Joint = CreateDefaultSubobject<USceneComponent>(JointName);
	Joint->SetupAttachment(Parent);
	Joint->SetRelativeLocation(Location);
	return Joint;
}

UStaticMeshComponent* ANACreature::MakePart(FName PartName, USceneComponent* Parent, UStaticMesh* Shape, const FVector& Location,
	const FVector& SizeCm, const FRotator& Rotation, bool bEmber, FName HitTag)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
	Part->SetupAttachment(Parent);
	Part->SetStaticMesh(Shape);
	Part->SetRelativeLocationAndRotation(Location, Rotation);
	Part->SetRelativeScale3D(SizeCm / NACreatureBody::ShapeSize);
	Part->SetGenerateOverlapEvents(false);
	if (ShapeMaterial.Succeeded())
	{
		Part->SetMaterial(0, ShapeMaterial.Object);
	}

	if (HitTag != NAME_None)
	{
		// Hittable by bullets only; never blocks movement.
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

void ANACreature::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANACreature, bSkinned);
	DOREPLIFETIME(ANACreature, Mood);
	DOREPLIFETIME(ANACreature, Prey);
	DOREPLIFETIME(ANACreature, BiteCount);
}

void ANACreature::BeginPlay()
{
	Super::BeginPlay();

	if (DenLocation.IsZero())
	{
		DenLocation = GetActorLocation();
	}

	for (UStaticMeshComponent* Part : AshParts)
	{
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), BodyColor);
		}
	}
	for (UStaticMeshComponent* Part : EmberParts)
	{
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), GlowColor);
		}
	}

	HealthComponent->OnDeathStateChanged.AddDynamic(this, &ANACreature::HandleDeathStateChanged);
	if (HasAuthority())
	{
		HealthComponent->OnKilled.AddDynamic(this, &ANACreature::HandleKilled);
	}
}

bool ANACreature::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

float ANACreature::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
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
	if (EventInstigator && EventInstigator->GetPawn())
	{
		LastAttacker = EventInstigator->GetPawn();
	}
	return HealthComponent->ApplyDamage(Damage, Zone, EventInstigator, DamageCauser);
}

void ANACreature::SetMood(ENACreatureMood NewMood, AActor* NewPrey)
{
	if (!HasAuthority())
	{
		return;
	}
	if (NewMood == ENACreatureMood::Hunting && Mood != ENACreatureMood::Hunting && GrowlSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, GrowlSound, GetActorLocation());
	}
	Mood = NewMood;
	Prey = NewPrey;
}

void ANACreature::SetMoveSpeed(float Speed)
{
	GetCharacterMovement()->MaxWalkSpeed = Speed;
}

bool ANACreature::TryBite(AActor* Victim)
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !Victim || IsDead() || !World)
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	if (Now - LastBiteTime < BiteCooldownSeconds)
	{
		return false;
	}

	const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
	if (ToVictim.Size() > BiteRange || FVector::DotProduct(GetActorForwardVector(), ToVictim.GetSafeNormal2D()) < 0.6f)
	{
		return false; // Out of reach or not facing it.
	}

	LastBiteTime = Now;
	UGameplayStatics::ApplyDamage(Victim, BiteDamage, GetController(), this, UDamageType::StaticClass());
	++BiteCount;
	OnRep_BiteCount(); // Play the lunge on the server/host too.
	return true;
}

void ANACreature::OnRep_BiteCount()
{
	if (GetWorld())
	{
		BiteAnimStartTime = GetWorld()->GetTimeSeconds();
	}
	if (BiteSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BiteSound, GetActorLocation());
	}
}

FName ANACreature::GetSkinItem() const
{
	return NAItems::AshHide();
}

void ANACreature::HandleKilled(AActor* Victim, AController* Killer)
{
	DropLoot();
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->OnCreatureKilled(this, Killer);
	}
}

void ANACreature::DropLoot()
{
	// Hunting rewards: hide, meat and a rare fang.
	// Dropped beside the body (not under it) so the carcass stays skinnable and the loot reachable.
	const FVector Side = GetActorRightVector() * 160.f;
	ANAItemPickup::Drop(GetWorld(), GetActorLocation() + Side, NAItems::AshHide(), 2);
	ANAItemPickup::Drop(GetWorld(), GetActorLocation() - Side, NAItems::RawMeat(), 3);
	if (FMath::FRand() < 0.6f)
	{
		ANAItemPickup::Drop(GetWorld(), GetActorLocation() + Side + GetActorForwardVector() * 80.f, NAItems::EmberFang(), 1);
	}
	if (FMath::FRand() < 0.15f)
	{
		ANAItemPickup::Drop(GetWorld(), GetActorLocation() - Side + GetActorForwardVector() * 80.f, TEXT("EnergyCell"), 1);
	}
}

void ANACreature::HandleDeathStateChanged(bool bNowDead)
{
	if (bNowDead)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		for (UStaticMeshComponent* Part : HitParts)
		{
			Part->SetCollisionResponseToChannel(NA_TRACE_WEAPON, ECR_Ignore);
		}
		EyeGlow->SetVisibility(false);
		if (DeathSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
		}
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		for (UStaticMeshComponent* Part : HitParts)
		{
			Part->SetCollisionResponseToChannel(NA_TRACE_WEAPON, ECR_Block);
		}
		EyeGlow->SetVisibility(true);
	}
}

FText ANACreature::GetInteractionPrompt(const ANACharacterBase* User) const
{
	const ANAPlayerCharacter* Player = Cast<ANAPlayerCharacter>(User);
	const UNAInventoryComponent* Inventory = Player ? Player->GetInventoryComponent() : nullptr;
	if (Inventory && Inventory->CountOf(NAItems::HuntingKnife()) > 0)
	{
		return FText::FromString(FString::Printf(TEXT("Skin the %s (extra hide and meat)"), *GetSpeciesName()));
	}
	return FText::FromString(TEXT("Skinning needs a Hunting Knife (craft: 2 Scrap + 1 Wood)"));
}

bool ANACreature::CanInteract(const ANACharacterBase* User) const
{
	return IsDead() && !bSkinned && Cast<ANAPlayerCharacter>(User) != nullptr;
}

void ANACreature::Interact(ANACharacterBase* User)
{
	ANAPlayerCharacter* Player = Cast<ANAPlayerCharacter>(User);
	UNAInventoryComponent* Inventory = Player ? Player->GetInventoryComponent() : nullptr;
	if (!HasAuthority() || !CanInteract(User) || !Inventory || Inventory->CountOf(NAItems::HuntingKnife()) <= 0)
	{
		return;
	}
	bSkinned = true;
	auto Give = [this, Inventory](FName Item, int32 Count)
	{
		const int32 Added = Inventory->AddItem(Item, Count);
		if (Added < Count)
		{
			ANAItemPickup::Drop(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 40.f), Item, Count - Added); // Inventory full.
		}
	};
	Give(GetSkinItem(), 1);
	Give(NAItems::RawMeat(), 2);
	if (GetSkinItem() == NAItems::AshHide() && FMath::FRand() < 0.25f)
	{
		Give(NAItems::EmberFang(), 1);
	}
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->AwardXP(Player->GetController(), 10, TEXT("Skinning"));
	}
}

void ANACreature::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Leave paw prints as it moves (server), so players can follow it.
	if (HasAuthority() && !IsDead() && GetCharacterMovement()->IsMovingOnGround())
	{
		const FVector Location = GetActorLocation();
		if (!bLeftFirstTrack || FVector::Dist2D(Location, LastTrackLocation) >= 450.f)
		{
			bLeftFirstTrack = true;
			LastTrackLocation = Location;
			bTrackLeft = !bTrackLeft;
			const FVector Ground = Location - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight())
				+ GetActorRightVector() * (bTrackLeft ? -15.f : 15.f);
			ANACreatureTrack::Spawn(GetWorld(), Ground, GetActorRotation().Yaw, GetSpeciesName());
		}
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		AnimateBody(DeltaSeconds); // Purely cosmetic: no need on a dedicated server.
	}
}

void ANACreature::AnimateBody(float DeltaSeconds)
{
	LifeTime += DeltaSeconds;

	if (IsDead())
	{
		// Collapse onto its side.
		const FRotator Current = VisualRoot->GetRelativeRotation();
		VisualRoot->SetRelativeRotation(FMath::RInterpTo(Current, FRotator(0.f, 0.f, 80.f), DeltaSeconds, 6.f));
		VisualRoot->SetRelativeLocation(FMath::VInterpTo(VisualRoot->GetRelativeLocation(), FVector(0.f, 0.f, -30.f), DeltaSeconds, 6.f));
		return;
	}

	const float Speed = GetVelocity().Size2D();
	const float Stride = FMath::Clamp(Speed / 350.f, 0.f, 1.f);
	GaitPhase += DeltaSeconds * (3.f + Speed / 45.f);

	// Legs: hips swing, knees fold while the foot is lifted.
	for (int32 Leg = 0; Leg < Hips.Num() && Leg < Knees.Num(); ++Leg)
	{
		const float Swing = FMath::Sin(GaitPhase + NACreatureBody::LegPhaseOffsets[Leg]);
		Hips[Leg]->SetRelativeRotation(FRotator(Swing * 32.f * Stride, 0.f, 0.f));
		Knees[Leg]->SetRelativeRotation(FRotator(-FMath::Max(0.f, Swing) * 40.f * Stride, 0.f, 0.f));
	}

	// Body: low crouch while stalking, gait bounce while moving, slow breathing at rest.
	const float Crouch = Mood == ENACreatureMood::Stalking ? -14.f : 0.f;
	const float Bounce = FMath::Abs(FMath::Sin(GaitPhase)) * 5.f * Stride;
	const float Breath = FMath::Sin(LifeTime * 2.f) * 1.5f * (1.f - Stride);
	const FVector TargetBody(0.f, 0.f, Crouch + Bounce + Breath);
	VisualRoot->SetRelativeLocation(FMath::VInterpTo(VisualRoot->GetRelativeLocation(), TargetBody, DeltaSeconds, 10.f));
	VisualRoot->SetRelativeRotation(FRotator::ZeroRotator);

	// Tail sways, faster and wider when agitated.
	const float Agitation = Mood == ENACreatureMood::Hunting ? 2.f : Mood == ENACreatureMood::Stalking ? 0.4f : 1.f;
	for (int32 Segment = 0; Segment < TailJoints.Num(); ++Segment)
	{
		const float Yaw = FMath::Sin(LifeTime * 2.2f * Agitation - Segment * 0.7f) * (6.f + Segment * 3.f) * Agitation;
		TailJoints[Segment]->SetRelativeRotation(FRotator(-4.f, Yaw, 0.f));
	}

	// Head tracks its prey; bite = lunge down/forward and open the jaw.
	float HeadYaw = 0.f;
	if (const AActor* Target = Prey)
	{
		const FVector Local = GetActorTransform().InverseTransformPosition(Target->GetActorLocation());
		HeadYaw = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X)), -45.f, 45.f);
	}
	const float SinceBite = GetWorld()->GetTimeSeconds() - BiteAnimStartTime;
	const float BiteAlpha = SinceBite < 0.35f ? FMath::Sin(SinceBite / 0.35f * PI) : 0.f;
	NeckJoint->SetRelativeLocation(FVector(88.f + BiteAlpha * 30.f, 0.f, 22.f - BiteAlpha * 6.f));
	NeckJoint->SetRelativeRotation(FMath::RInterpTo(NeckJoint->GetRelativeRotation(), FRotator(-BiteAlpha * 22.f, HeadYaw, 0.f), DeltaSeconds, 12.f));
	JawJoint->SetRelativeRotation(FRotator(-BiteAlpha * 38.f - (Mood == ENACreatureMood::Hunting ? 8.f : 0.f), 0.f, 0.f));

	// Eyes flare when hunting.
	EyeGlow->SetIntensity(Mood == ENACreatureMood::Hunting ? 8000.f : 3000.f);
}
