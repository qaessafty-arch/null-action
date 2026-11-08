#include "World/NAPrecipitation.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace NAPrecip
{
	constexpr int32 RainCount = 700;
	constexpr int32 SnowCount = 600;
	constexpr int32 DustCount = 400;
	constexpr float HalfWidth = 1600.f;
	constexpr float HalfHeight = 1000.f;
}

ANAPrecipitation::ANAPrecipitation()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	auto MakeLayer = [this, SceneRoot](const TCHAR* LayerName, UStaticMesh* Shape, UMaterialInterface* Material)
	{
		UInstancedStaticMeshComponent* Layer = CreateDefaultSubobject<UInstancedStaticMeshComponent>(LayerName);
		Layer->SetupAttachment(SceneRoot);
		Layer->SetStaticMesh(Shape);
		Layer->SetMaterial(0, Material);
		Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Layer->SetCastShadow(false);
		Layer->SetMobility(EComponentMobility::Movable);
		return Layer;
	};
	RainDrops = MakeLayer(TEXT("RainDrops"), Cylinder.Object, ShapeMaterial.Object);
	SnowFlakes = MakeLayer(TEXT("SnowFlakes"), Cube.Object, ShapeMaterial.Object);
	DustMotes = MakeLayer(TEXT("DustMotes"), Cube.Object, ShapeMaterial.Object);
}

void ANAPrecipitation::BeginPlay()
{
	Super::BeginPlay();
	auto Tint = [](UInstancedStaticMeshComponent* Layer, const FLinearColor& Color)
	{
		if (UMaterialInstanceDynamic* MID = Layer->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	};
	Tint(RainDrops, FLinearColor(0.55f, 0.62f, 0.7f));
	Tint(SnowFlakes, FLinearColor(0.95f, 0.95f, 1.f));
	Tint(DustMotes, FLinearColor(0.45f, 0.32f, 0.15f));
}

void ANAPrecipitation::SetIntensity(float NewRain, float NewSnow, float NewDust)
{
	RainAmount = FMath::Clamp(NewRain, 0.f, 1.f);
	SnowAmount = FMath::Clamp(NewSnow, 0.f, 1.f);
	DustAmount = FMath::Clamp(NewDust, 0.f, 1.f);
}

void ANAPrecipitation::InitLayer(FLayer& Layer, UInstancedStaticMeshComponent* Component, int32 Count, const FVector& Center)
{
	Layer.Positions.SetNum(Count);
	Layer.Transforms.SetNum(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Layer.Positions[Index] = Center + FVector(FMath::FRandRange(-NAPrecip::HalfWidth, NAPrecip::HalfWidth),
			FMath::FRandRange(-NAPrecip::HalfWidth, NAPrecip::HalfWidth), FMath::FRandRange(-NAPrecip::HalfHeight, NAPrecip::HalfHeight));
		Layer.Transforms[Index] = FTransform(FRotator::ZeroRotator, Layer.Positions[Index], FVector::ZeroVector);
		Component->AddInstance(Layer.Transforms[Index], /*bWorldSpace*/ true);
	}
}

void ANAPrecipitation::UpdateLayer(FLayer& Layer, UInstancedStaticMeshComponent* Component, float Amount, const FVector& Center,
	const FVector& Velocity, const FVector& Scale, float DeltaSeconds, bool bWobble)
{
	const int32 Count = Layer.Positions.Num();
	const int32 Visible = FMath::RoundToInt(Count * Amount);
	if (Visible == 0 && Component->GetInstanceCount() > 0 && Layer.Transforms.Num() > 0 && Layer.Transforms[0].GetScale3D().IsZero())
	{
		return; // Already hidden; nothing to update.
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		FVector& Position = Layer.Positions[Index];
		Position += Velocity * DeltaSeconds;
		if (bWobble)
		{
			Position.X += FMath::Sin(Time * 1.7f + Index) * 30.f * DeltaSeconds;
			Position.Y += FMath::Cos(Time * 1.3f + Index * 0.7f) * 30.f * DeltaSeconds;
		}

		// Keep every particle inside a box that follows the camera (wrap around the edges).
		FVector Offset = Position - Center;
		if (Offset.Z < -NAPrecip::HalfHeight) { Offset.Z += 2.f * NAPrecip::HalfHeight; }
		if (Offset.Z > NAPrecip::HalfHeight)  { Offset.Z -= 2.f * NAPrecip::HalfHeight; }
		if (Offset.X < -NAPrecip::HalfWidth)  { Offset.X += 2.f * NAPrecip::HalfWidth; }
		if (Offset.X > NAPrecip::HalfWidth)   { Offset.X -= 2.f * NAPrecip::HalfWidth; }
		if (Offset.Y < -NAPrecip::HalfWidth)  { Offset.Y += 2.f * NAPrecip::HalfWidth; }
		if (Offset.Y > NAPrecip::HalfWidth)   { Offset.Y -= 2.f * NAPrecip::HalfWidth; }
		Position = Center + Offset;

		Layer.Transforms[Index] = FTransform(FRotator::ZeroRotator, Position, Index < Visible ? Scale : FVector::ZeroVector);
	}
	Component->BatchUpdateInstancesTransforms(0, Layer.Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
}

void ANAPrecipitation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;

	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const FVector Center = PC->PlayerCameraManager->GetCameraLocation();

	if (!bInitialised)
	{
		InitLayer(Rain, RainDrops, NAPrecip::RainCount, Center);
		InitLayer(Snow, SnowFlakes, NAPrecip::SnowCount, Center);
		InitLayer(Dust, DustMotes, NAPrecip::DustCount, Center);
		bInitialised = true;
	}

	UpdateLayer(Rain, RainDrops, RainAmount, Center, FVector(60.f, 30.f, -1400.f), FVector(0.006f, 0.006f, 0.55f), DeltaSeconds, false);
	UpdateLayer(Snow, SnowFlakes, SnowAmount, Center, FVector(40.f, 20.f, -160.f) * (1.f + SnowAmount), FVector(0.035f), DeltaSeconds, true);
	UpdateLayer(Dust, DustMotes, DustAmount, Center, FVector(900.f, 350.f, -20.f), FVector(0.025f), DeltaSeconds, true);
}
