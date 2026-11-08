#include "World/NAEventActors.h"

#include "AI/NAEnemySoldier.h"
#include "Character/NAPlayerCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAGameMode.h"
#include "Core/NAPlayerController.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace NAEventShapes
{
	UStaticMeshComponent* Make(AActor* Owner, FName Name, USceneComponent* Parent, const TCHAR* MeshPath, const FVector& Location,
		const FVector& SizeCm, const FRotator& Rotation, bool bCollide)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		if (Parent) { Part->SetupAttachment(Parent); }
		if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath)) { Part->SetStaticMesh(Mesh); }
		if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			Part->SetMaterial(0, Material);
		}
		Part->SetRelativeLocationAndRotation(Location, Rotation);
		Part->SetRelativeScale3D(SizeCm / 100.f);
		if (bCollide)
		{
			Part->SetCollisionProfileName(TEXT("BlockAll"));
		}
		else
		{
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return Part;
	}

	void Paint(UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		if (UMaterialInstanceDynamic* MID = Part ? Part->CreateAndSetMaterialInstanceDynamic(0) : nullptr)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}

	float ServerTime(const UWorld* World)
	{
		const AGameStateBase* State = World ? World->GetGameState() : nullptr;
		return State ? static_cast<float>(State->GetServerWorldTimeSeconds()) : 0.f;
	}
}

// ----- Ember strike ------------------------------------------------------------------------------

ANAEmberStrike::ANAEmberStrike()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	InitialLifeSpan = Delay + 1.5f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Disc = NAEventShapes::Make(this, TEXT("Disc"), RootComponent, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(0.f, 0.f, 3.f),
		FVector(Radius * 2.f, Radius * 2.f, 2.f), FRotator::ZeroRotator, false);
	Disc->CastShadow = false;
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(RootComponent);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	Glow->SetLightColor(FLinearColor(1.f, 0.35f, 0.08f));
	Glow->SetIntensity(0.f);
	Glow->SetAttenuationRadius(900.f);
	Glow->SetCastShadows(false);
}

void ANAEmberStrike::BeginPlay()
{
	Super::BeginPlay();
	NAEventShapes::Paint(Disc, FLinearColor(0.9f, 0.08f, 0.02f));
}

void ANAEmberStrike::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	if (!bBurst)
	{
		const float Pulse = 0.85f + 0.15f * FMath::Sin(Age * (8.f + Age * 12.f));
		Disc->SetRelativeScale3D(FVector(Radius * 2.f / 100.f * Pulse, Radius * 2.f / 100.f * Pulse, 0.02f));
	}
	if (!bBurst && Age >= Delay)
	{
		bBurst = true;
		Disc->SetVisibility(false);
		Glow->SetIntensity(60000.f); // The flash of the ember burst.
		if (HasAuthority())
		{
			TArray<AActor*> Ignored;
			UGameplayStatics::ApplyRadialDamage(this, Damage, GetActorLocation() + FVector(0.f, 0.f, 50.f), Radius, UDamageType::StaticClass(),
				Ignored, this, nullptr, true, ECC_Visibility);
		}
	}
	if (bBurst)
	{
		Glow->SetIntensity(FMath::Max(0.f, Glow->Intensity - 80000.f * DeltaSeconds));
	}
}

// ----- Distress beacon ---------------------------------------------------------------------------

ANADistressBeacon::ANADistressBeacon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Mast = NAEventShapes::Make(this, TEXT("Mast"), RootComponent, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(0.f, 0.f, 110.f),
		FVector(14.f, 14.f, 220.f), FRotator::ZeroRotator, true);
	Lamp = NAEventShapes::Make(this, TEXT("Lamp"), RootComponent, TEXT("/Engine/BasicShapes/Sphere.Sphere"), FVector(0.f, 0.f, 232.f),
		FVector(26.f), FRotator::ZeroRotator, false);
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(RootComponent);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 240.f));
	Light->SetLightColor(FLinearColor(1.f, 0.1f, 0.05f));
	Light->SetIntensity(4000.f);
	Light->SetAttenuationRadius(1500.f);
	Light->SetCastShadows(false);
}

void ANADistressBeacon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANADistressBeacon, bActive);
	DOREPLIFETIME(ANADistressBeacon, bDone);
	DOREPLIFETIME(ANADistressBeacon, TransmitEndServerTime);
}

void ANADistressBeacon::BeginPlay()
{
	Super::BeginPlay();
	NAEventShapes::Paint(Mast, FLinearColor(0.2f, 0.2f, 0.22f));
	NAEventShapes::Paint(Lamp, FLinearColor(1.f, 0.1f, 0.05f));
}

float ANADistressBeacon::GetTransmitRemaining() const
{
	return bActive ? FMath::Max(0.f, TransmitEndServerTime - NAEventShapes::ServerTime(GetWorld())) : 0.f;
}

FText ANADistressBeacon::GetInteractionPrompt(const ANACharacterBase* User) const
{
	return FText::FromString(TEXT("Activate the distress beacon (it will attract enemies)"));
}

void ANADistressBeacon::Interact(ANACharacterBase* User)
{
	if (!HasAuthority() || bActive || bDone || !User)
	{
		return;
	}
	bActive = true;
	Activator = User->GetController();
	TransmitEndServerTime = NAEventShapes::ServerTime(GetWorld()) + TransmitSeconds;

	// The signal draws a squad: three soldiers arrive from different sides.
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float Angle = Index * 120.f + FMath::FRandRange(-20.f, 20.f);
		const FVector Offset = FRotator(0.f, Angle, 0.f).Vector() * 1800.f;
		const FTransform Where(FRotator(0.f, Angle + 180.f, 0.f), GetActorLocation() + Offset + FVector(0.f, 0.f, 150.f));
		if (ANAEnemySoldier* Soldier = GetWorld()->SpawnActorDeferred<ANAEnemySoldier>(ANAEnemySoldier::StaticClass(), Where, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
		{
			Soldier->FinishSpawning(Where);
			Soldier->SetLifeSpan(240.f);
		}
	}
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->AnnounceToAll(TEXT("The distress beacon is transmitting. Hold the area for 45 seconds!"));
	}
}

void ANADistressBeacon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Now = GetWorld()->GetTimeSeconds();
	Light->SetIntensity(bActive ? (FMath::Fmod(Now, 0.8f) < 0.4f ? 12000.f : 1000.f) : 4000.f); // Blinks while transmitting.

	if (!HasAuthority() || !bActive || bDone || GetTransmitRemaining() > 0.f)
	{
		return;
	}
	bActive = false;
	bDone = true;
	AController* Holder = nullptr;
	for (TActorIterator<ANAPlayerCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead() && !It->IsDowned() && FVector::Dist(It->GetActorLocation(), GetActorLocation()) < 2000.f)
		{
			Holder = It->GetController();
			break;
		}
	}
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->OnBeaconFinished(this, Holder);
	}
	SetLifeSpan(30.f);
}

// ----- Shipwreck ---------------------------------------------------------------------------------

ANAShipwreck::ANAShipwreck()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* Cylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	// A listing hull: floor, two broken sides, a stern wall, a snapped mast lying across.
	Planks.Add(NAEventShapes::Make(this, TEXT("Keel"), RootComponent, Cube, FVector(0.f, 0.f, 20.f), FVector(1400.f, 420.f, 40.f), FRotator(0.f, 0.f, 6.f), true));
	Planks.Add(NAEventShapes::Make(this, TEXT("SidePort"), RootComponent, Cube, FVector(0.f, -220.f, 140.f), FVector(1300.f, 30.f, 240.f), FRotator(0.f, 0.f, -18.f), true));
	Planks.Add(NAEventShapes::Make(this, TEXT("SideStarboard"), RootComponent, Cube, FVector(-150.f, 220.f, 100.f), FVector(900.f, 30.f, 160.f), FRotator(0.f, 0.f, 25.f), true));
	Planks.Add(NAEventShapes::Make(this, TEXT("Stern"), RootComponent, Cube, FVector(-690.f, 0.f, 160.f), FVector(30.f, 420.f, 280.f), FRotator::ZeroRotator, true));
	Planks.Add(NAEventShapes::Make(this, TEXT("Bow"), RootComponent, Cube, FVector(720.f, 0.f, 120.f), FVector(160.f, 300.f, 200.f), FRotator(0.f, 45.f, 0.f), true));
	Planks.Add(NAEventShapes::Make(this, TEXT("Mast"), RootComponent, Cylinder, FVector(100.f, 300.f, 120.f), FVector(30.f, 30.f, 900.f), FRotator(80.f, 30.f, 0.f), true));
}

void ANAShipwreck::BeginPlay()
{
	Super::BeginPlay();
	for (UStaticMeshComponent* Plank : Planks)
	{
		NAEventShapes::Paint(Plank, FLinearColor(0.1f, 0.075f, 0.05f)); // Waterlogged wood.
	}
}

FTransform ANAShipwreck::CrateTransform() const
{
	return FTransform(GetActorRotation(), GetActorTransform().TransformPosition(FVector(-450.f, 0.f, 45.f)));
}
