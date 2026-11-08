#include "World/NATerrainManager.h"

#include "NullAction.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

// ----- World shape --------------------------------------------------------------------------------

namespace NATerrainDetail
{
	float SmoothStep(float Edge0, float Edge1, float X)
	{
		const float T = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}

	float Noise(float X, float Y, float Scale, const FVector2D& Offset)
	{
		return FMath::PerlinNoise2D(FVector2D(X / Scale, Y / Scale) + Offset);
	}

	enum ESurface : int32 { Grass, ForestFloor, Rock, Snow, Sand, SurfaceCount };

	const FLinearColor SurfaceColors[SurfaceCount] =
	{
		FLinearColor(0.16f, 0.30f, 0.08f), // Grass
		FLinearColor(0.08f, 0.14f, 0.05f), // Forest floor
		FLinearColor(0.22f, 0.21f, 0.20f), // Rock
		FLinearColor(0.85f, 0.87f, 0.92f), // Snow
		FLinearColor(0.42f, 0.36f, 0.24f), // Sand / lake mud
	};

	ESurface SurfaceAt(float X, float Y, float Height, const FVector& Normal)
	{
		if (Height < NATerrain::WaterLevel() + 60.f) { return Sand; }
		if (FVector2D::Distance(FVector2D(X, Y), NATerrain::CraterCenter()) < 3000.f) { return Rock; } // Ash floor.
		if (Height > 5200.f + Noise(X, Y, 3000.f, FVector2D(5.3f, 1.7f)) * 600.f) { return Snow; }
		if (Normal.Z < 0.78f) { return Rock; }
		return NATerrain::ForestDensityAt(X, Y) > 0.5f ? ForestFloor : Grass;
	}
}

float NATerrain::HeightAt(float X, float Y)
{
	using namespace NATerrainDetail;

	// Rolling hills at three scales.
	float Height = Noise(X, Y, 18000.f, FVector2D(17.3f, 41.9f)) * 900.f
		+ Noise(X, Y, 6000.f, FVector2D(-8.1f, 3.3f)) * 260.f
		+ Noise(X, Y, 1500.f, FVector2D(2.7f, -19.4f)) * 35.f + 150.f;

	// Grey Peaks: sharp ridged mountains rising to the north (+X).
	const float MountainMask = SmoothStep(14000.f, 38000.f, X);
	const float Ridge = 1.f - FMath::Abs(Noise(X, Y, 11000.f, FVector2D(-31.7f, 12.1f)));
	Height += Ridge * Ridge * 9500.f * MountainMask;

	// Mirror Lake: a basin below the water line.
	const float LakeDistance = FVector2D::Distance(FVector2D(X, Y), LakeCenter());
	Height = FMath::Lerp(Height, -900.f, 1.f - SmoothStep(4500.f, 9500.f, LakeDistance));

	// Flat ground around the test arena (square, matching its walls) and the cabin pad.
	const float ArenaDistance = FMath::Max(FMath::Abs(X), FMath::Abs(Y));
	Height = FMath::Lerp(Height, -5.f, 1.f - SmoothStep(4600.f, 7500.f, ArenaDistance));
	const float CabinDistance = FVector2D::Distance(FVector2D(X, Y), CabinCenter());
	Height = FMath::Lerp(Height, -5.f, 1.f - SmoothStep(1100.f, 2200.f, CabinDistance));

	// Ash Crater: a flat bowl with a ring-shaped rim (boss arena).
	const float CraterDistance = FVector2D::Distance(FVector2D(X, Y), CraterCenter());
	Height = FMath::Lerp(Height, -120.f, 1.f - SmoothStep(2400.f, 3800.f, CraterDistance));
	const float RimOffset = (CraterDistance - 3200.f) / 450.f;
	Height += 380.f * FMath::Exp(-RimOffset * RimOffset);

	return Height;
}

FVector NATerrain::NormalAt(float X, float Y)
{
	constexpr float Step = 100.f;
	const float DX = HeightAt(X + Step, Y) - HeightAt(X - Step, Y);
	const float DY = HeightAt(X, Y + Step) - HeightAt(X, Y - Step);
	return FVector(-DX, -DY, 2.f * Step).GetSafeNormal();
}

float NATerrain::ForestDensityAt(float X, float Y)
{
	using namespace NATerrainDetail;
	float Density = Noise(X, Y, 9000.f, FVector2D(51.1f, -7.7f)) * 0.6f + 0.45f;
	Density += 0.3f * SmoothStep(0.f, -18000.f, X); // Whispering Pines: denser to the south-west.
	const float ArenaDistance = FMath::Max(FMath::Abs(X), FMath::Abs(Y));
	Density *= SmoothStep(5000.f, 7000.f, ArenaDistance);
	Density *= SmoothStep(1800.f, 2600.f, FVector2D::Distance(FVector2D(X, Y), CabinCenter()));
	Density *= SmoothStep(3600.f, 4400.f, FVector2D::Distance(FVector2D(X, Y), CraterCenter())); // Nothing grows in the crater.
	return FMath::Clamp(Density, 0.f, 1.f);
}

FString NATerrain::RegionAt(const FVector& Location)
{
	const float X = Location.X;
	const float Y = Location.Y;
	if (FMath::Max(FMath::Abs(X), FMath::Abs(Y)) < 4100.f)                       { return TEXT("Test Arena"); }
	if (FVector2D::Distance(FVector2D(X, Y), CabinCenter()) < 1800.f)            { return TEXT("Old Cabin"); }
	if (FVector2D::Distance(FVector2D(X, Y), CraterCenter()) < 3600.f)           { return TEXT("Ash Crater"); }
	if (FVector2D::Distance(FVector2D(X, Y), LakeCenter()) < 9500.f)             { return TEXT("Mirror Lake"); }
	if (X > 16000.f || HeightAt(X, Y) > 2500.f)                                  { return TEXT("Grey Peaks"); }
	if (ForestDensityAt(X, Y) > 0.55f)                                           { return TEXT("Whispering Pines"); }
	return TEXT("Ashen Grassland");
}

// ----- Streaming ----------------------------------------------------------------------------------

ANATerrainManager::ANATerrainManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	bReplicates = true;   // Exists on every machine; each builds its own chunks locally.
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	SphereShape = Sphere.Object;
	BaseMaterial = ShapeMaterial.Object;
	CylinderShape = Cylinder.Object;
	ConeShape = Cone.Object;
	CubeShape = Cube.Object;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ANATerrainManager::BeginPlay()
{
	Super::BeginPlay();

	auto MakeMaterial = [this](const FLinearColor& Color)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
		return MID;
	};
	for (int32 Index = 0; Index < NATerrainDetail::SurfaceCount; ++Index)
	{
		SurfaceMaterials.Add(MakeMaterial(NATerrainDetail::SurfaceColors[Index]));
	}
	BarkMaterial = MakeMaterial(FLinearColor(0.12f, 0.07f, 0.04f));
	NeedleMaterial = MakeMaterial(FLinearColor(0.03f, 0.10f, 0.04f));
	StoneMaterial = MakeMaterial(FLinearColor(0.25f, 0.24f, 0.23f));
	GrassMaterial = MakeMaterial(FLinearColor(0.09f, 0.16f, 0.05f));
	BushMaterial = MakeMaterial(FLinearColor(0.05f, 0.11f, 0.04f));
}

void ANATerrainManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (TPair<FIntPoint, FNATerrainChunk>& Pair : Chunks)
	{
		DestroyChunk(Pair.Value);
	}
	Chunks.Empty();
	Super::EndPlay(EndPlayReason);
}

void ANATerrainManager::GatherViewpoints(TArray<TPair<FVector, int32>>& OutViewpoints) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Server: every pawn (players and AI) needs ground and tree collision under it.
	if (HasAuthority())
	{
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			OutViewpoints.Add(TPair<FVector, int32>(It->GetActorLocation(), ServerRadiusChunks));
		}
	}

	// Anyone with a screen: the full view distance around the local camera.
	if (GetNetMode() != NM_DedicatedServer)
	{
		const APlayerController* PC = World->GetFirstPlayerController();
		if (PC && PC->PlayerCameraManager)
		{
			OutViewpoints.Add(TPair<FVector, int32>(PC->PlayerCameraManager->GetCameraLocation(), ViewRadiusChunks));
		}
	}
}

void ANATerrainManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TArray<TPair<FVector, int32>> Viewpoints;
	GatherViewpoints(Viewpoints);
	if (Viewpoints.Num() == 0)
	{
		return;
	}

	// Which chunks should exist, and how close is each to its nearest viewer?
	TMap<FIntPoint, float> Wanted;
	for (const TPair<FVector, int32>& View : Viewpoints)
	{
		const FIntPoint Center(FMath::FloorToInt(View.Key.X / ChunkSize), FMath::FloorToInt(View.Key.Y / ChunkSize));
		const int32 Radius = View.Value;
		for (int32 DX = -Radius; DX <= Radius; ++DX)
		{
			for (int32 DY = -Radius; DY <= Radius; ++DY)
			{
				if (DX * DX + DY * DY > Radius * Radius + 1)
				{
					continue;
				}
				const FIntPoint Key(Center.X + DX, Center.Y + DY);
				const float Distance = FMath::Sqrt(static_cast<float>(DX * DX + DY * DY));
				float& Best = Wanted.FindOrAdd(Key, TNumericLimits<float>::Max());
				Best = FMath::Min(Best, Distance);
			}
		}
	}

	// Build the closest missing chunks first, a few per frame to avoid hitches.
	TArray<TPair<float, FIntPoint>> Missing;
	for (const TPair<FIntPoint, float>& Pair : Wanted)
	{
		if (!Chunks.Contains(Pair.Key))
		{
			Missing.Add(TPair<float, FIntPoint>(Pair.Value, Pair.Key));
		}
	}
	Missing.Sort([](const TPair<float, FIntPoint>& A, const TPair<float, FIntPoint>& B) { return A.Key < B.Key; });
	for (int32 Index = 0; Index < Missing.Num() && Index < MaxChunkBuildsPerFrame; ++Index)
	{
		BuildChunk(Missing[Index].Value);
	}

	// Drop chunks nobody needs any more (with a margin, so walking back and forth doesn't thrash).
	TArray<FIntPoint> ToRemove;
	for (const TPair<FIntPoint, FNATerrainChunk>& Pair : Chunks)
	{
		if (Wanted.Contains(Pair.Key))
		{
			continue;
		}
		bool bNearAnyone = false;
		for (const TPair<FVector, int32>& View : Viewpoints)
		{
			const FIntPoint Center(FMath::FloorToInt(View.Key.X / ChunkSize), FMath::FloorToInt(View.Key.Y / ChunkSize));
			if (FMath::Abs(Pair.Key.X - Center.X) <= View.Value + 2 && FMath::Abs(Pair.Key.Y - Center.Y) <= View.Value + 2)
			{
				bNearAnyone = true;
				break;
			}
		}
		if (!bNearAnyone)
		{
			ToRemove.Add(Pair.Key);
		}
	}
	for (const FIntPoint& Key : ToRemove)
	{
		DestroyChunk(Chunks[Key]);
		Chunks.Remove(Key);
	}
}

void ANATerrainManager::BuildChunk(const FIntPoint& Key)
{
	FNATerrainChunk& Chunk = Chunks.Add(Key);
	BuildGround(Chunk, Key);
	BuildScatter(Chunk, Key);
	BuildGroundCover(Chunk, Key);
}

void ANATerrainManager::BuildGround(FNATerrainChunk& Chunk, const FIntPoint& Key)
{
	using namespace NATerrainDetail;

	UProceduralMeshComponent* Ground = NewObject<UProceduralMeshComponent>(this);
	Ground->SetupAttachment(RootComponent);
	Ground->bUseAsyncCooking = true; // Cook collision off the game thread.
	Ground->SetCastShadow(true);
	Ground->RegisterComponent();
	Ground->SetWorldLocation(FVector(Key.X * ChunkSize, Key.Y * ChunkSize, 0.f));

	const int32 N = Resolution + 1;
	const float Step = ChunkSize / Resolution;
	const float OriginX = Key.X * ChunkSize;
	const float OriginY = Key.Y * ChunkSize;

	// Sample the grid once.
	TArray<FVector> GridPositions;
	TArray<FVector> GridNormals;
	GridPositions.SetNum(N * N);
	GridNormals.SetNum(N * N);
	for (int32 Y = 0; Y < N; ++Y)
	{
		for (int32 X = 0; X < N; ++X)
		{
			const float WorldX = OriginX + X * Step;
			const float WorldY = OriginY + Y * Step;
			GridPositions[Y * N + X] = FVector(X * Step, Y * Step, NATerrain::HeightAt(WorldX, WorldY));
			GridNormals[Y * N + X] = NATerrain::NormalAt(WorldX, WorldY);
		}
	}

	// One mesh section per surface type, so each gets its own (lit) material.
	struct FSection
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TMap<int32, int32> Remap;
	};
	FSection Sections[SurfaceCount];

	auto AddVertex = [&](FSection& Section, int32 GridIndex)
	{
		if (const int32* Existing = Section.Remap.Find(GridIndex))
		{
			return *Existing;
		}
		const int32 NewIndex = Section.Vertices.Add(GridPositions[GridIndex]);
		Section.Normals.Add(GridNormals[GridIndex]);
		Section.UVs.Add(FVector2D(GridPositions[GridIndex].X, GridPositions[GridIndex].Y) / 400.f);
		Section.Remap.Add(GridIndex, NewIndex);
		return NewIndex;
	};

	for (int32 Y = 0; Y < Resolution; ++Y)
	{
		for (int32 X = 0; X < Resolution; ++X)
		{
			const int32 I00 = Y * N + X;
			const int32 I10 = I00 + 1;
			const int32 I01 = I00 + N;
			const int32 I11 = I01 + 1;
			const FVector Middle = (GridPositions[I00] + GridPositions[I11]) * 0.5f;
			const FVector MiddleNormal = (GridNormals[I00] + GridNormals[I11]).GetSafeNormal();
			FSection& Section = Sections[SurfaceAt(OriginX + Middle.X, OriginY + Middle.Y, Middle.Z, MiddleNormal)];

			const int32 A = AddVertex(Section, I00);
			const int32 B = AddVertex(Section, I10);
			const int32 C = AddVertex(Section, I01);
			const int32 D = AddVertex(Section, I11);
			Section.Triangles.Append({ A, C, B, B, C, D });
		}
	}

	const TArray<FLinearColor> NoColors;
	const TArray<FProcMeshTangent> NoTangents;
	for (int32 Surface = 0; Surface < SurfaceCount; ++Surface)
	{
		FSection& Section = Sections[Surface];
		if (Section.Triangles.Num() == 0)
		{
			continue;
		}
		Ground->CreateMeshSection_LinearColor(Surface, Section.Vertices, Section.Triangles, Section.Normals, Section.UVs, NoColors, NoTangents, /*bCreateCollision*/ true);
		if (SurfaceMaterials.IsValidIndex(Surface))
		{
			Ground->SetMaterial(Surface, SurfaceMaterials[Surface]);
		}
	}
	Ground->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Chunk.Ground = Ground;
}

UInstancedStaticMeshComponent* ANATerrainManager::MakeInstances(UStaticMesh* Shape, UMaterialInterface* Material, bool bCollision)
{
	UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(this);
	Instances->SetupAttachment(RootComponent);
	Instances->SetStaticMesh(Shape);
	Instances->SetMaterial(0, Material);
	Instances->SetMobility(EComponentMobility::Movable);
	if (bCollision)
	{
		Instances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	else
	{
		Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	Instances->RegisterComponent();
	return Instances;
}

void ANATerrainManager::BuildScatter(FNATerrainChunk& Chunk, const FIntPoint& Key)
{
	const bool bVisuals = GetNetMode() != NM_DedicatedServer;
	Chunk.Trunks = MakeInstances(CylinderShape, BarkMaterial, true);
	Chunk.Rocks = MakeInstances(CubeShape, StoneMaterial, true);
	Chunk.Trunks->ComponentTags.Add(TEXT("NA.Tree"));  // Gatherable: wood.
	Chunk.Rocks->ComponentTags.Add(TEXT("NA.Rock"));   // Gatherable: stone.
	if (bVisuals)
	{
		Chunk.Foliage = MakeInstances(ConeShape, NeedleMaterial, false);
	}

	// 8 x 8 cells per chunk; each cell may hold one tree, decided by a hash of its world cell.
	constexpr int32 Cells = 8;
	const float CellSize = ChunkSize / Cells;
	for (int32 CY = 0; CY < Cells; ++CY)
	{
		for (int32 CX = 0; CX < Cells; ++CX)
		{
			const int32 WorldCellX = Key.X * Cells + CX;
			const int32 WorldCellY = Key.Y * Cells + CY;
			FRandomStream Random(static_cast<int32>(HashCombine(GetTypeHash(WorldCellX), GetTypeHash(WorldCellY * 7919 + 13))));

			const float X = (WorldCellX + Random.FRandRange(0.15f, 0.85f)) * CellSize;
			const float Y = (WorldCellY + Random.FRandRange(0.15f, 0.85f)) * CellSize;
			const float Z = NATerrain::HeightAt(X, Y);
			const FVector Normal = NATerrain::NormalAt(X, Y);
			if (Z < NATerrain::WaterLevel() + 80.f || Normal.Z < 0.8f)
			{
				continue; // No trees in the lake or on cliffs.
			}

			const float Roll = Random.FRand();
			const float TreeLine = 4200.f;
			if (Z < TreeLine && Roll < NATerrain::ForestDensityAt(X, Y) * 0.9f)
			{
				// Pine: trunk plus two stacked cones.
				const float TreeHeight = Random.FRandRange(900.f, 1700.f);
				const float Width = TreeHeight * Random.FRandRange(0.22f, 0.3f);
				Chunk.Trunks->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, Z + TreeHeight * 0.5f - 30.f),
					FVector(0.35f, 0.35f, TreeHeight / 100.f)), true);
				if (Chunk.Foliage)
				{
					Chunk.Foliage->AddInstance(FTransform(FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f), FVector(X, Y, Z + TreeHeight * 0.55f),
						FVector(Width / 100.f, Width / 100.f, TreeHeight * 0.55f / 100.f)), true);
					Chunk.Foliage->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, Z + TreeHeight * 0.85f),
						FVector(Width * 0.7f / 100.f, Width * 0.7f / 100.f, TreeHeight * 0.4f / 100.f)), true);
				}
			}
			else if (Roll > 0.93f)
			{
				// Boulder, half buried.
				const FVector Size(Random.FRandRange(80.f, 320.f), Random.FRandRange(80.f, 320.f), Random.FRandRange(60.f, 220.f));
				Chunk.Rocks->AddInstance(FTransform(FRotator(Random.FRandRange(-15.f, 15.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-15.f, 15.f)),
					FVector(X, Y, Z + Size.Z * 0.25f), Size / 100.f), true);
			}
		}
	}
}

void ANATerrainManager::BuildGroundCover(FNATerrainChunk& Chunk, const FIntPoint& Key)
{
	if (GetNetMode() == NM_DedicatedServer || !ConeShape || !SphereShape)
	{
		return; // Purely visual.
	}
	Chunk.Grass = MakeInstances(ConeShape, GrassMaterial, false);
	Chunk.Bushes = MakeInstances(SphereShape, BushMaterial, false);
	Chunk.Grass->SetCullDistances(0, 6500);
	Chunk.Grass->SetCastShadow(false);
	Chunk.Bushes->SetCullDistances(0, 12000);

	// Grass grows in patches (noise): thick and tall in some places, short and sparse in others.
	constexpr int32 Cells = 32;
	const float CellSize = ChunkSize / Cells;
	FRandomStream Random(static_cast<int32>(HashCombine(GetTypeHash(Key.X * 31 + 7), GetTypeHash(Key.Y * 977 + 3))));
	for (int32 CY = 0; CY < Cells; ++CY)
	{
		for (int32 CX = 0; CX < Cells; ++CX)
		{
			const float X = Key.X * ChunkSize + (CX + Random.FRandRange(0.f, 1.f)) * CellSize;
			const float Y = Key.Y * ChunkSize + (CY + Random.FRandRange(0.f, 1.f)) * CellSize;
			const float Z = NATerrain::HeightAt(X, Y);
			const FVector Normal = NATerrain::NormalAt(X, Y);
			// Keep clear of the arena floor, the cabin pad, the crater ash, water, cliffs and high snow.
			if (FMath::Max(FMath::Abs(X), FMath::Abs(Y)) < 4300.f ||
				FVector2D::Distance(FVector2D(X, Y), NATerrain::CabinCenter()) < 1000.f ||
				FVector2D::Distance(FVector2D(X, Y), NATerrain::CraterCenter()) < 3100.f ||
				Z < NATerrain::WaterLevel() + 70.f || Z > 4500.f || Normal.Z < 0.85f)
			{
				continue;
			}
			const float Patch = NATerrainDetail::Noise(X, Y, 2500.f, FVector2D(3.1f, -9.7f)); // -1..1
			if (Random.FRand() > 0.55f + Patch * 0.35f)
			{
				continue;
			}
			const float Height = Random.FRandRange(25.f, 55.f) * (Patch > 0.f ? 1.f : 0.6f);
			const FRotator Tilt(Random.FRandRange(-12.f, 12.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-12.f, 12.f));
			Chunk.Grass->AddInstance(FTransform(Tilt, FVector(X, Y, Z + Height * 0.45f), FVector(0.07f, 0.07f, Height / 100.f)), true);

			// Occasional low bush, more of them near the forest edge.
			if (Random.FRand() < 0.025f + NATerrain::ForestDensityAt(X, Y) * 0.05f)
			{
				const float Size = Random.FRandRange(70.f, 160.f);
				Chunk.Bushes->AddInstance(FTransform(FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f), FVector(X, Y, Z + Size * 0.3f),
					FVector(Size, Size * Random.FRandRange(0.8f, 1.2f), Size * 0.7f) / 100.f), true);
			}
		}
	}
}

bool ANATerrainManager::TryHarvest(const FVector& Location)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const FIntPoint Cell(FMath::RoundToInt(Location.X / 200.f), FMath::RoundToInt(Location.Y / 200.f));
	FHarvestState& State = HarvestStates.FindOrAdd(Cell);
	if (State.HitsLeft <= 0)
	{
		if (Now < State.RegrowTime)
		{
			return false;
		}
		State.HitsLeft = 5;
	}
	if (--State.HitsLeft <= 0)
	{
		State.RegrowTime = Now + 180.f;
	}
	return true;
}

void ANATerrainManager::DestroyChunk(FNATerrainChunk& Chunk)
{
	if (Chunk.Ground)  { Chunk.Ground->DestroyComponent(); }
	if (Chunk.Trunks)  { Chunk.Trunks->DestroyComponent(); }
	if (Chunk.Foliage) { Chunk.Foliage->DestroyComponent(); }
	if (Chunk.Rocks)   { Chunk.Rocks->DestroyComponent(); }
	if (Chunk.Grass)   { Chunk.Grass->DestroyComponent(); }
	if (Chunk.Bushes)  { Chunk.Bushes->DestroyComponent(); }
	Chunk = FNATerrainChunk();
}
