#include "World/NATestArena.h"

#include "NullAction.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace NAArena
{
	constexpr float CubeSize = 100.f; // Engine cube is 100 cm, pivot at centre.
	constexpr float FloorHalfExtent = 4000.f;

	const FLinearColor Palette[] =
	{
		FLinearColor(0.08f, 0.08f, 0.09f), // 0 graphite
		FLinearColor(0.75f, 0.08f, 0.05f), // 1 NULL-ACTION red
		FLinearColor(0.18f, 0.28f, 0.40f), // 2 steel blue
		FLinearColor(0.55f, 0.47f, 0.33f), // 3 sand
	};
}

ANATestArena::ANATestArena()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Grid(TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Shape(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	GridMaterial = Grid.Object;
	ShapeMaterial = Shape.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	const float E = NAArena::FloorHalfExtent;

	// Floor: 80 m x 80 m, top surface at Z = 0.
	AddBox(TEXT("Floor"), FVector(0.f, 0.f, -100.f), FVector(E * 2.f, E * 2.f, 100.f), FRotator::ZeroRotator, -1);

	// Perimeter walls.
	AddBox(TEXT("WallN"), FVector( E, 0.f, 0.f), FVector(50.f, E * 2.f, 150.f), FRotator::ZeroRotator, 0);
	AddBox(TEXT("WallS"), FVector(-E, 0.f, 0.f), FVector(50.f, E * 2.f, 150.f), FRotator::ZeroRotator, 0);
	AddBox(TEXT("WallE"), FVector(0.f,  E, 0.f), FVector(E * 2.f, 50.f, 150.f), FRotator::ZeroRotator, 0);
	AddBox(TEXT("WallW"), FVector(0.f, -E, 0.f), FVector(E * 2.f, 50.f, 150.f), FRotator::ZeroRotator, 0);

	// Lane 1 (Y = -1200): stairs, 8 steps of 25 cm, then a landing.
	for (int32 Step = 0; Step < 8; ++Step)
	{
		const FName Name(*FString::Printf(TEXT("Stair_%d"), Step));
		AddBox(Name, FVector(800.f + 40.f * Step + 20.f, -1200.f, 0.f), FVector(40.f, 400.f, 25.f * (Step + 1)), FRotator::ZeroRotator, 3);
	}
	AddBox(TEXT("StairLanding"), FVector(1320.f, -1200.f, 0.f), FVector(400.f, 400.f, 200.f), FRotator::ZeroRotator, 3);

	// Lane 2 (Y = -600): 18-degree ramp up to a platform.
	{
		const float RampLength = 800.f;
		const float Angle = 18.f;
		const float Rise = RampLength * FMath::Sin(FMath::DegreesToRadians(Angle));
		const float Run = RampLength * FMath::Cos(FMath::DegreesToRadians(Angle));
		AddBox(TEXT("Ramp"), FVector(800.f + Run * 0.5f, -600.f, Rise * 0.5f - 10.f), FVector(RampLength, 400.f, 20.f), FRotator(Angle, 0.f, 0.f), 2);
		AddBox(TEXT("RampPlatform"), FVector(800.f + Run + 200.f, -600.f, 0.f), FVector(400.f, 400.f, Rise), FRotator::ZeroRotator, 2);
	}

	// Lane 3 (Y = 0): jump boxes. 40 and 80 cm are jumpable; 120 cm is not (future vault/climb test).
	AddBox(TEXT("JumpBox_40"),  FVector( 800.f, 0.f, 0.f), FVector(200.f, 200.f,  40.f), FRotator::ZeroRotator, 1);
	AddBox(TEXT("JumpBox_80"),  FVector(1150.f, 0.f, 0.f), FVector(200.f, 200.f,  80.f), FRotator::ZeroRotator, 1);
	AddBox(TEXT("JumpBox_120"), FVector(1500.f, 0.f, 0.f), FVector(200.f, 200.f, 120.f), FRotator::ZeroRotator, 1);

	// Lane 4 (Y = 600): crouch tunnel. Roof underside at 130 cm: standing (180) blocked, crouched (110) fits.
	AddBox(TEXT("TunnelWallL"), FVector(1200.f, 450.f, 0.f), FVector(800.f, 40.f, 200.f), FRotator::ZeroRotator, 0);
	AddBox(TEXT("TunnelWallR"), FVector(1200.f, 750.f, 0.f), FVector(800.f, 40.f, 200.f), FRotator::ZeroRotator, 0);
	AddBox(TEXT("TunnelRoof"),  FVector(1200.f, 600.f, 130.f), FVector(800.f, 340.f, 30.f), FRotator::ZeroRotator, 0);

	// Behind spawn: pillars and a long wall to exercise camera collision.
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const FName Name(*FString::Printf(TEXT("Pillar_%d"), Index));
		AddBox(Name, FVector(-800.f, -750.f + 300.f * Index, 0.f), FVector(60.f, 60.f, 400.f), FRotator::ZeroRotator, 2);
	}
	AddBox(TEXT("CameraWall"), FVector(-1500.f, 0.f, 0.f), FVector(50.f, 1600.f, 500.f), FRotator::ZeroRotator, 1);

	// Combat yard (X 2000..3700, Y -3700..-1300): low cover (crouch behind, stand to peek), crates, a tall wall.
	AddBox(TEXT("Yard_Barrier"),   FVector(2700.f, -1450.f, 0.f), FVector(400.f, 40.f, 100.f), FRotator::ZeroRotator, 0);
	AddBox(TEXT("Yard_CoverA"),    FVector(2350.f, -1900.f, 0.f), FVector(40.f, 500.f, 130.f), FRotator::ZeroRotator, 0);
	AddBox(TEXT("Yard_CoverB"),    FVector(2900.f, -2600.f, 0.f), FVector(500.f, 40.f, 130.f), FRotator::ZeroRotator, 0);
	AddBox(TEXT("Yard_Crate1"),    FVector(2650.f, -2300.f, 0.f), FVector(120.f, 120.f, 110.f), FRotator(0.f, 15.f, 0.f), 3);
	AddBox(TEXT("Yard_Crate2"),    FVector(3300.f, -1850.f, 0.f), FVector(120.f, 120.f, 110.f), FRotator::ZeroRotator, 3);
	AddBox(TEXT("Yard_Crate3"),    FVector(3100.f, -3100.f, 0.f), FVector(150.f, 150.f, 120.f), FRotator(0.f, -20.f, 0.f), 3);
	AddBox(TEXT("Yard_Crate4"),    FVector(2350.f, -3000.f, 0.f), FVector(120.f, 240.f, 110.f), FRotator::ZeroRotator, 3);
	AddBox(TEXT("Yard_TallWall"),  FVector(3550.f, -2500.f, 0.f), FVector(40.f, 900.f, 300.f), FRotator::ZeroRotator, 1);
	AddBox(TEXT("Yard_Container"), FVector(2650.f, -3450.f, 0.f), FVector(600.f, 240.f, 260.f), FRotator::ZeroRotator, 2);

	// Ash Stalker den (back-left corner): a ring of dark rocks with a gap facing the arena.
	AddBox(TEXT("Den_Rock1"), FVector(-3300.f, 2600.f, 0.f), FVector(260.f, 200.f, 160.f), FRotator(0.f, 20.f, 0.f), 0);
	AddBox(TEXT("Den_Rock2"), FVector(-3100.f, 3300.f, 0.f), FVector(300.f, 220.f, 220.f), FRotator(0.f, -15.f, 0.f), 0);
	AddBox(TEXT("Den_Rock3"), FVector(-2500.f, 3400.f, 0.f), FVector(240.f, 260.f, 140.f), FRotator(0.f, 40.f, 0.f), 0);
	AddBox(TEXT("Den_Rock4"), FVector(-3500.f, 3050.f, 0.f), FVector(180.f, 240.f, 260.f), FRotator(0.f, 5.f, 0.f), 0);
	AddBox(TEXT("Den_Bones"), FVector(-2850.f, 2950.f, 0.f), FVector(90.f, 20.f, 12.f), FRotator(0.f, 60.f, 0.f), 3);

	// Environment lighting so an empty level is lit. Removed in BeginPlay if the level has its own sun.
	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetRelativeRotation(FRotator(-38.f, 35.f, 0.f));
	Sun->SetIntensity(8.f);
	Sun->SetAtmosphereSunLight(true);

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(Root);
}

UStaticMeshComponent* ANATestArena::AddBox(FName Name, const FVector& BottomCentre, const FVector& Size, const FRotator& Rotation, int32 ColorIndex)
{
	UStaticMeshComponent* Box = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Box->SetupAttachment(Root);
	Box->SetStaticMesh(CubeMesh);
	Box->SetMaterial(0, ColorIndex < 0 ? GridMaterial.Get() : ShapeMaterial.Get());
	Box->SetRelativeLocationAndRotation(BottomCentre + FVector(0.f, 0.f, Size.Z * 0.5f), Rotation);
	Box->SetRelativeScale3D(Size / NAArena::CubeSize);
	Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Box->SetCanEverAffectNavigation(true);

	Blocks.Add(Box);
	BlockColors.Add(ColorIndex);
	return Box;
}

void ANATestArena::BeginPlay()
{
	Super::BeginPlay();

	// Tint blocks per lane (runs on every machine; materials are cosmetic).
	for (int32 Index = 0; Index < Blocks.Num(); ++Index)
	{
		const int32 ColorIndex = BlockColors.IsValidIndex(Index) ? BlockColors[Index] : -1;
		if (ColorIndex >= 0 && Blocks[Index])
		{
			if (UMaterialInstanceDynamic* MID = Blocks[Index]->CreateAndSetMaterialInstanceDynamic(0))
			{
				MID->SetVectorParameterValue(TEXT("Color"), NAArena::Palette[ColorIndex % static_cast<int32>(UE_ARRAY_COUNT(NAArena::Palette))]);
			}
		}
	}

	TActorIterator<ADirectionalLight> ExistingSun(GetWorld());
	const bool bLevelHasSun = static_cast<bool>(ExistingSun);

	if (!bProvideLighting || bLevelHasSun)
	{
		Sun->DestroyComponent();
		Atmosphere->DestroyComponent();
		SkyLight->DestroyComponent();
		Fog->DestroyComponent();
		UE_LOG(LogNullAction, Log, TEXT("Test arena: using the level's own lighting."));
	}
}

FTransform ANATestArena::GetSpawnTransform(int32 Index) const
{
	constexpr int32 Slots = 8;
	constexpr float Radius = 250.f;
	const float Angle = (2.f * PI / Slots) * (Index % Slots);
	const FVector Local(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 110.f);
	// Everyone faces +X, toward the movement course.
	return FTransform(GetActorRotation(), GetActorTransform().TransformPosition(Local));
}
