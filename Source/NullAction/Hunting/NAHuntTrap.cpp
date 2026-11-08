#include "Hunting/NAHuntTrap.h"

#include "AI/NAEnemySoldier.h"
#include "Character/NACharacterBase.h"
#include "Character/NAPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Creatures/NACreature.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ANAHuntTrap::ANAHuntTrap()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;
	Trigger->SetBoxExtent(FVector(55.f, 55.f, 40.f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);

	auto MakeShape = [this](FName Name, USceneComponent* Parent, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->CastShadow = false;
		return Part;
	};
	Plate = MakeShape(TEXT("Plate"), Trigger, FVector(0.f, 0.f, -38.f), FVector(0.7f, 0.7f, 0.03f));
	HingeA = CreateDefaultSubobject<USceneComponent>(TEXT("HingeA"));
	HingeA->SetupAttachment(Trigger);
	HingeA->SetRelativeLocation(FVector(0.f, -35.f, -36.f));
	HingeB = CreateDefaultSubobject<USceneComponent>(TEXT("HingeB"));
	HingeB->SetupAttachment(Trigger);
	HingeB->SetRelativeLocation(FVector(0.f, 35.f, -36.f));
	// Open jaws lie flat; they swing up to meet in the middle when sprung.
	JawA = MakeShape(TEXT("JawA"), HingeA, FVector(0.f, -18.f, 0.f), FVector(0.65f, 0.36f, 0.04f));
	JawB = MakeShape(TEXT("JawB"), HingeB, FVector(0.f, 18.f, 0.f), FVector(0.65f, 0.36f, 0.04f));

	if (Cylinder.Succeeded()) { Plate->SetStaticMesh(Cylinder.Object); }
	if (Cube.Succeeded())
	{
		JawA->SetStaticMesh(Cube.Object);
		JawB->SetStaticMesh(Cube.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		Plate->SetMaterial(0, ShapeMaterial.Object);
		JawA->SetMaterial(0, ShapeMaterial.Object);
		JawB->SetMaterial(0, ShapeMaterial.Object);
	}
}

void ANAHuntTrap::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAHuntTrap, bSprung);
}

void ANAHuntTrap::BeginPlay()
{
	Super::BeginPlay();
	const FLinearColor Rust(0.12f, 0.09f, 0.07f); // Dark: hard to spot on the ground.
	for (UStaticMeshComponent* Part : { Plate.Get(), JawA.Get(), JawB.Get() })
	{
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Rust);
		}
	}
	if (HasAuthority())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &ANAHuntTrap::OnTriggerOverlap);
	}
}

ANAHuntTrap* ANAHuntTrap::Place(ANACharacterBase* User)
{
	UWorld* World = User ? User->GetWorld() : nullptr;
	if (!World || !User->HasAuthority())
	{
		return nullptr;
	}
	int32 Armed = 0;
	for (TActorIterator<ANAHuntTrap> It(World); It; ++It)
	{
		if (It->Placer.Get() == User && !It->bSprung) { ++Armed; }
	}
	if (Armed >= MaxTrapsPerPlayer)
	{
		return nullptr;
	}

	const FVector Ahead = User->GetActorLocation() + User->GetActorForwardVector().GetSafeNormal2D() * 130.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NATrapPlace), false, User);
	FHitResult Ground;
	if (!World->LineTraceSingleByChannel(Ground, Ahead + FVector(0.f, 0.f, 50.f), Ahead - FVector(0.f, 0.f, 300.f), ECC_Visibility, Params) ||
		Ground.ImpactNormal.Z < 0.7f)
	{
		return nullptr; // Needs fairly flat ground.
	}
	const FTransform Where(FRotator(0.f, User->GetActorRotation().Yaw, 0.f), Ground.ImpactPoint + FVector(0.f, 0.f, 40.f));
	ANAHuntTrap* Trap = World->SpawnActorDeferred<ANAHuntTrap>(ANAHuntTrap::StaticClass(), Where, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Trap)
	{
		Trap->Placer = User;
		Trap->FinishSpawning(Where);
	}
	return Trap;
}

void ANAHuntTrap::OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bSprung || !OtherActor || OtherActor == Placer.Get())
	{
		return;
	}
	if (const ANACreature* Creature = Cast<ANACreature>(OtherActor))
	{
		if (!Creature->IsDead()) { Spring(OtherActor); }
	}
	else if (const ANACharacterBase* Body = Cast<ANACharacterBase>(OtherActor))
	{
		if (!Body->IsDead() && !Body->IsDowned()) { Spring(OtherActor); }
	}
}

void ANAHuntTrap::Spring(AActor* Victim)
{
	bSprung = true;
	OnRep_Sprung();
	SetLifeSpan(20.f);

	APawn* Setter = Placer.Get();
	UGameplayStatics::ApplyDamage(Victim, Damage, Setter ? Setter->GetController() : nullptr, this, UDamageType::StaticClass());

	// Hold creatures and AI soldiers in place.
	ACharacter* Held = Cast<ACharacter>(Victim);
	if (Held && !Held->IsA<ANAPlayerCharacter>())
	{
		UCharacterMovementComponent* Move = Held->GetCharacterMovement();
		Move->StopMovementImmediately();
		Move->DisableMovement();
		TWeakObjectPtr<ACharacter> WeakHeld(Held);
		FTimerHandle Release;
		GetWorldTimerManager().SetTimer(Release, FTimerDelegate::CreateLambda([WeakHeld]()
		{
			ACharacter* Freed = WeakHeld.Get();
			if (!Freed) { return; }
			const ANACreature* Creature = Cast<ANACreature>(Freed);
			const ANACharacterBase* Body = Cast<ANACharacterBase>(Freed);
			const bool bAlive = (Creature && !Creature->IsDead()) || (Body && !Body->IsDead());
			if (bAlive) { Freed->GetCharacterMovement()->SetMovementMode(MOVE_Walking); }
		}), HoldSeconds, false);
	}
}

void ANAHuntTrap::OnRep_Sprung()
{
	if (bSprung)
	{
		HingeA->SetRelativeRotation(FRotator(0.f, 0.f, 80.f));
		HingeB->SetRelativeRotation(FRotator(0.f, 0.f, -80.f));
	}
}
