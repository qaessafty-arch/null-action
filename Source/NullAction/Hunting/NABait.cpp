#include "Hunting/NABait.h"

#include "Character/NACharacterBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ANABait::ANABait()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	InitialLifeSpan = 120.f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Meat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Meat"));
	RootComponent = Meat;
	Meat->SetRelativeScale3D(FVector(0.4f, 0.3f, 0.2f));
	Meat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Sphere.Succeeded()) { Meat->SetStaticMesh(Sphere.Object); }
	if (ShapeMaterial.Succeeded()) { Meat->SetMaterial(0, ShapeMaterial.Object); }
}

void ANABait::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInstanceDynamic* MID = Meat->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.08f, 0.06f));
	}
}

ANABait* ANABait::Place(ANACharacterBase* User)
{
	UWorld* World = User ? User->GetWorld() : nullptr;
	if (!World || !User->HasAuthority())
	{
		return nullptr;
	}
	const FVector Ahead = User->GetActorLocation() + User->GetActorForwardVector().GetSafeNormal2D() * 120.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NABaitPlace), false, User);
	FHitResult Ground;
	if (!World->LineTraceSingleByChannel(Ground, Ahead + FVector(0.f, 0.f, 50.f), Ahead - FVector(0.f, 0.f, 300.f), ECC_Visibility, Params))
	{
		return nullptr;
	}
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ANABait>(ANABait::StaticClass(), FTransform(Ground.ImpactPoint + FVector(0.f, 0.f, 10.f)), SpawnParams);
}

ANABait* ANABait::FindNearest(UWorld* World, const FVector& Location, float Radius)
{
	ANABait* Best = nullptr;
	float BestDistance = Radius;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ANABait> It(World); It; ++It)
	{
		const float Distance = FVector::Dist(It->GetActorLocation(), Location);
		if (Distance < BestDistance && !It->IsActorBeingDestroyed())
		{
			Best = *It;
			BestDistance = Distance;
		}
	}
	return Best;
}
