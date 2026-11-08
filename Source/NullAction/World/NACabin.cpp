#include "World/NACabin.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace NACabinColors
{
	const FLinearColor Logs(0.16f, 0.09f, 0.05f);
	const FLinearColor DarkWood(0.08f, 0.05f, 0.03f);
	const FLinearColor Floor(0.22f, 0.15f, 0.09f);
	const FLinearColor Roof(0.1f, 0.1f, 0.11f);
	const FLinearColor Fabric(0.35f, 0.1f, 0.08f);
	const FLinearColor Metal(0.3f, 0.3f, 0.32f);
	const FLinearColor Stone(0.25f, 0.24f, 0.23f);
}

ANACabin::ANACabin()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	ShapeMaterial = Material.Object;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	using namespace NACabinColors;

	// Floor, foundation and roof (10 m x 7 m, 3 m walls).
	AddBox(TEXT("Foundation"), FVector(0.f, 0.f, -40.f), FVector(1060.f, 760.f, 40.f), Stone);
	AddBox(TEXT("Floor"), FVector(0.f, 0.f, -2.f), FVector(1000.f, 700.f, 4.f), Floor);
	AddBox(TEXT("Roof"), FVector(0.f, 0.f, 300.f), FVector(1100.f, 800.f, 25.f), Roof);
	AddBox(TEXT("Chimney"), FVector(110.f, 310.f, 300.f), FVector(70.f, 70.f, 160.f), Stone);
	AddBox(TEXT("Porch"), FVector(620.f, 0.f, -40.f), FVector(240.f, 400.f, 40.f), DarkWood);

	// Front wall (+X) with the front doorway (1.2 m wide, 2.25 m high).
	AddBox(TEXT("FrontL"), FVector(500.f, -205.f, 0.f), FVector(20.f, 290.f, 300.f), Logs);
	AddBox(TEXT("FrontR"), FVector(500.f, 205.f, 0.f), FVector(20.f, 290.f, 300.f), Logs);
	AddBox(TEXT("FrontTop"), FVector(500.f, 0.f, 225.f), FVector(20.f, 120.f, 75.f), Logs);

	// Back wall (-X), solid.
	AddBox(TEXT("Back"), FVector(-500.f, 0.f, 0.f), FVector(20.f, 700.f, 300.f), Logs);

	// Side walls (+/-Y) with a window (4 m wide, 0.9-2.1 m high) in each.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Y = Side == 0 ? 350.f : -350.f;
		const TCHAR* Prefix = Side == 0 ? TEXT("East") : TEXT("West");
		AddBox(*FString::Printf(TEXT("%sLow"), Prefix),   FVector(0.f, Y, 0.f),   FVector(1000.f, 20.f, 90.f), Logs);
		AddBox(*FString::Printf(TEXT("%sHigh"), Prefix),  FVector(0.f, Y, 210.f), FVector(1000.f, 20.f, 90.f), Logs);
		AddBox(*FString::Printf(TEXT("%sFront"), Prefix), FVector(350.f, Y, 90.f), FVector(300.f, 20.f, 120.f), Logs);
		AddBox(*FString::Printf(TEXT("%sBack"), Prefix),  FVector(-350.f, Y, 90.f), FVector(300.f, 20.f, 120.f), Logs);
	}

	// Interior wall across the middle (x = 0) with doorways to the bedroom (+Y) and storage (-Y).
	AddBox(TEXT("MidA"), FVector(0.f, -287.5f, 0.f), FVector(16.f, 125.f, 300.f), DarkWood);
	AddBox(TEXT("MidB"), FVector(0.f, 0.f, 0.f), FVector(16.f, 250.f, 300.f), DarkWood);
	AddBox(TEXT("MidC"), FVector(0.f, 287.5f, 0.f), FVector(16.f, 125.f, 300.f), DarkWood);
	AddBox(TEXT("MidTopA"), FVector(0.f, -175.f, 225.f), FVector(16.f, 100.f, 75.f), DarkWood);
	AddBox(TEXT("MidTopB"), FVector(0.f, 175.f, 225.f), FVector(16.f, 100.f, 75.f), DarkWood);
	// Wall between bedroom and storage.
	AddBox(TEXT("Split"), FVector(-250.f, 0.f, 0.f), FVector(500.f, 16.f, 300.f), DarkWood);

	// Living room (front half): table, chairs, bench, shelf, fireplace.
	AddBox(TEXT("Table"), FVector(250.f, 120.f, 0.f), FVector(160.f, 90.f, 75.f), DarkWood);
	AddBox(TEXT("ChairA"), FVector(250.f, 220.f, 0.f), FVector(45.f, 45.f, 45.f), DarkWood);
	AddBox(TEXT("ChairB"), FVector(250.f, 20.f, 0.f), FVector(45.f, 45.f, 45.f), DarkWood);
	AddBox(TEXT("Bench"), FVector(120.f, -280.f, 0.f), FVector(200.f, 60.f, 45.f), Logs);
	AddBox(TEXT("Shelf"), FVector(420.f, -320.f, 0.f), FVector(120.f, 35.f, 190.f), DarkWood);
	AddBox(TEXT("Fireplace"), FVector(110.f, 300.f, 0.f), FVector(120.f, 60.f, 110.f), Stone);

	// Bedroom (back, +Y): bed with blanket, wardrobe, nightstand.
	AddBox(TEXT("BedFrame"), FVector(-330.f, 220.f, 0.f), FVector(210.f, 110.f, 45.f), DarkWood);
	AddBox(TEXT("Blanket"), FVector(-345.f, 220.f, 45.f), FVector(170.f, 112.f, 10.f), Fabric);
	AddBox(TEXT("Wardrobe"), FVector(-120.f, 310.f, 0.f), FVector(120.f, 55.f, 210.f), DarkWood);
	AddBox(TEXT("Nightstand"), FVector(-460.f, 120.f, 0.f), FVector(50.f, 50.f, 55.f), DarkWood);

	// Storage (back, -Y): metal shelving and old barrels.
	AddBox(TEXT("RackA"), FVector(-300.f, -320.f, 0.f), FVector(300.f, 45.f, 200.f), Metal);
	AddBox(TEXT("BarrelA"), FVector(-210.f, -250.f, 0.f), FVector(60.f, 60.f, 90.f), Logs);
	AddBox(TEXT("BarrelB"), FVector(-60.f, -310.f, 0.f), FVector(60.f, 60.f, 90.f), Logs);

	// Warm interior lighting.
	LivingLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LivingLight"));
	LivingLight->SetupAttachment(RootComponent);
	LivingLight->SetRelativeLocation(FVector(250.f, 0.f, 250.f));
	LivingLight->SetIntensity(8000.f);
	LivingLight->SetAttenuationRadius(700.f);
	LivingLight->SetLightColor(FLinearColor(1.f, 0.7f, 0.4f));

	BedroomLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BedroomLight"));
	BedroomLight->SetupAttachment(RootComponent);
	BedroomLight->SetRelativeLocation(FVector(-250.f, 175.f, 240.f));
	BedroomLight->SetIntensity(3000.f);
	BedroomLight->SetAttenuationRadius(450.f);
	BedroomLight->SetLightColor(FLinearColor(1.f, 0.65f, 0.35f));
}

void ANACabin::AddBox(const TCHAR* PartName, const FVector& BottomCentre, const FVector& Size, const FLinearColor& Color, float Yaw)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
	Part->SetupAttachment(RootComponent);
	Part->SetStaticMesh(CubeMesh);
	Part->SetMaterial(0, ShapeMaterial);
	Part->SetRelativeLocationAndRotation(BottomCentre + FVector(0.f, 0.f, Size.Z * 0.5f), FRotator(0.f, Yaw, 0.f));
	Part->SetRelativeScale3D(Size / 100.f);
	Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Parts.Add(Part);
	PartColors.Add(Color);
}

void ANACabin::BeginPlay()
{
	Super::BeginPlay();
	for (int32 Index = 0; Index < Parts.Num(); ++Index)
	{
		if (Parts[Index] && PartColors.IsValidIndex(Index))
		{
			if (UMaterialInstanceDynamic* MID = Parts[Index]->CreateAndSetMaterialInstanceDynamic(0))
			{
				MID->SetVectorParameterValue(TEXT("Color"), PartColors[Index]);
			}
		}
	}
}

FTransform ANACabin::FrontDoorTransform() const
{
	// Hinge at the left edge of the front doorway; the panel extends toward +Y.
	return FTransform(FRotator::ZeroRotator, FVector(500.f, -60.f, 0.f)) * GetActorTransform();
}

FTransform ANACabin::StorageDoorTransform() const
{
	return FTransform(FRotator::ZeroRotator, FVector(0.f, -225.f, 0.f)) * GetActorTransform();
}

FTransform ANACabin::CrateTransform() const
{
	return FTransform(FRotator(0.f, 10.f, 0.f), FVector(-380.f, -150.f, 27.f)) * GetActorTransform();
}

FTransform ANACabin::NoteTransform() const
{
	return FTransform(FRotator(0.f, 20.f, 0.f), FVector(250.f, 120.f, 76.f)) * GetActorTransform();
}

FTransform ANACabin::MedkitTransform() const
{
	return FTransform(FRotator::ZeroRotator, FVector(-460.f, 120.f, 110.f)) * GetActorTransform();
}
