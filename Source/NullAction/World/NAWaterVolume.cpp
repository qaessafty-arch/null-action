#include "World/NAWaterVolume.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ANAWaterVolume::ANAWaterVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	// Thin plane at the surface; it never blocks anything.
	Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Surface"));
	Surface->SetupAttachment(SceneRoot);
	Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Surface->SetCastShadow(false);
	if (Cube.Succeeded()) { Surface->SetStaticMesh(Cube.Object); }
	if (ShapeMaterial.Succeeded()) { Surface->SetMaterial(0, ShapeMaterial.Object); }
	Surface->SetRelativeLocation(FVector(0.f, 0.f, Extent.Z));
	Surface->SetRelativeScale3D(FVector(Extent.X * 2.f / 100.f, Extent.Y * 2.f / 100.f, 0.02f));
}

void ANAWaterVolume::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInstanceDynamic* MID = Surface->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.12f, 0.2f));
	}
}

bool ANAWaterVolume::GetSurfaceAt(const FVector& Point, float& OutSurfaceZ) const
{
	const FVector Local = Point - GetActorLocation();
	if (FMath::Abs(Local.X) > Extent.X || FMath::Abs(Local.Y) > Extent.Y || Local.Z < -Extent.Z - 60.f || Local.Z > Extent.Z + 150.f)
	{
		return false;
	}
	OutSurfaceZ = GetActorLocation().Z + Extent.Z;
	return true;
}

bool ANAWaterVolume::FindWaterAt(const UWorld* World, const FVector& Point, float& OutSurfaceZ)
{
	if (!World)
	{
		return false;
	}
	for (TActorIterator<ANAWaterVolume> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->GetSurfaceAt(Point, OutSurfaceZ))
		{
			return true;
		}
	}
	return false;
}
