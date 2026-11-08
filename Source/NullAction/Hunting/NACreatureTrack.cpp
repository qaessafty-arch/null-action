#include "Hunting/NACreatureTrack.h"

#include "Character/NACharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAPlayerController.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ANACreatureTrack::ANACreatureTrack()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(1.f); // Never changes after it appears.
	SetNetCullDistanceSquared(FMath::Square(6000.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Only the look-trace hits it, so players can read it but never trip over it.
	Hitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	RootComponent = Hitbox;
	Hitbox->SetBoxExtent(FVector(30.f, 30.f, 4.f));
	Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	Hitbox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	PrintLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrintLeft"));
	PrintLeft->SetupAttachment(Hitbox);
	PrintLeft->SetRelativeLocation(FVector(8.f, -12.f, -2.f));
	PrintRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrintRight"));
	PrintRight->SetupAttachment(Hitbox);
	PrintRight->SetRelativeLocation(FVector(-8.f, 12.f, -2.f));
	for (UStaticMeshComponent* Print : { PrintLeft.Get(), PrintRight.Get() })
	{
		Print->SetRelativeScale3D(FVector(0.22f, 0.14f, 0.02f));
		Print->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Print->CastShadow = false;
		if (Sphere.Succeeded()) { Print->SetStaticMesh(Sphere.Object); }
		if (ShapeMaterial.Succeeded()) { Print->SetMaterial(0, ShapeMaterial.Object); }
	}
}

void ANACreatureTrack::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ANACreatureTrack, SpawnServerTime, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ANACreatureTrack, CreatureName, COND_InitialOnly);
}

void ANACreatureTrack::BeginPlay()
{
	Super::BeginPlay();
	for (UStaticMeshComponent* Print : { PrintLeft.Get(), PrintRight.Get() })
	{
		if (UMaterialInstanceDynamic* MID = Print->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.04f, 0.035f));
		}
	}
}

ANACreatureTrack* ANACreatureTrack::Spawn(UWorld* World, const FVector& GroundLocation, float Yaw, const FString& Name)
{
	if (!World)
	{
		return nullptr;
	}
	const FTransform Where(FRotator(0.f, Yaw, 0.f), GroundLocation + FVector(0.f, 0.f, 3.f));
	ANACreatureTrack* Track = World->SpawnActorDeferred<ANACreatureTrack>(ANACreatureTrack::StaticClass(), Where, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Track)
	{
		const AGameStateBase* State = World->GetGameState();
		Track->SpawnServerTime = State ? static_cast<float>(State->GetServerWorldTimeSeconds()) : 0.f;
		Track->CreatureName = Name;
		Track->SetLifeSpan(LifeSeconds);
		Track->FinishSpawning(Where);
	}
	return Track;
}

float ANACreatureTrack::GetAgeSeconds() const
{
	const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return State ? FMath::Max(0.f, static_cast<float>(State->GetServerWorldTimeSeconds()) - SpawnServerTime) : 0.f;
}

FString ANACreatureTrack::CompassName(float Yaw)
{
	// +X is north, +Y is east.
	static const TCHAR* Names[] = { TEXT("north"), TEXT("north-east"), TEXT("east"), TEXT("south-east"),
		TEXT("south"), TEXT("south-west"), TEXT("west"), TEXT("north-west") };
	const float Normalized = FMath::Fmod(Yaw + 360.f + 22.5f, 360.f);
	return Names[FMath::Clamp(static_cast<int32>(Normalized / 45.f), 0, 7)];
}

FText ANACreatureTrack::GetInteractionPrompt(const ANACharacterBase* User) const
{
	const float Age = GetAgeSeconds();
	const TCHAR* Freshness = Age < 20.f ? TEXT("very fresh") : Age < 50.f ? TEXT("fresh") : TEXT("old");
	return FText::FromString(FString::Printf(TEXT("%s tracks - %s (%.0f s), heading %s"),
		*CreatureName, Freshness, Age, *CompassName(GetActorRotation().Yaw)));
}

void ANACreatureTrack::Interact(ANACharacterBase* User)
{
	ANAPlayerController* Reader = User ? Cast<ANAPlayerController>(User->GetController()) : nullptr;
	if (Reader)
	{
		Reader->ClientReceiveChat(ENAChatChannel::System, FString(), FString::Printf(TEXT("You study the prints: %s."),
			*GetInteractionPrompt(User).ToString()));
	}
}
