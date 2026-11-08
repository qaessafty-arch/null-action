#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NATerrainManager.generated.h"

class UProceduralMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;

/**
 * Deterministic world shape. Pure functions of position, so the server and every client compute
 * exactly the same ground without sending any terrain over the network.
 */
namespace NATerrain
{
	NULLACTION_API float HeightAt(float X, float Y);
	NULLACTION_API FVector NormalAt(float X, float Y);
	NULLACTION_API float ForestDensityAt(float X, float Y);
	NULLACTION_API FString RegionAt(const FVector& Location);

	inline float WaterLevel() { return -300.f; }
	inline FVector2D LakeCenter() { return FVector2D(-15000.f, 15000.f); }
	inline FVector2D CabinCenter() { return FVector2D(7000.f, -1500.f); }
	/** Ash Crater: the Ashen Behemoth's lair (flat ash floor with a raised rim). */
	inline FVector2D CraterCenter() { return FVector2D(12500.f, 6500.f); }
}

USTRUCT()
struct FNATerrainChunk
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Ground;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Trunks;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Foliage;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rocks;
	/** Cosmetic ground cover (clients only, no collision, culled at a distance). */
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Grass;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Bushes;
	/** Real tree / rock meshes from Project Settings -> NULL-ACTION Assets (one component per mesh). */
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> RealTrees;
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> RealRocks;
};

/**
 * Streams the open world in square chunks around every player (and, on the server, every AI pawn),
 * and removes chunks nobody is near. Each chunk is generated ground (with collision) plus pine
 * trees and rocks placed from a deterministic hash, so everyone sees the same forest.
 *
 * This is the runtime stand-in for an authored World Partition landscape: the same idea
 * (only nearby cells exist in memory), built from code so it works without editor-made assets.
 */
UCLASS(NotPlaceable)
class NULLACTION_API ANATerrainManager : public AActor
{
	GENERATED_BODY()

public:
	ANATerrainManager();

	virtual void Tick(float DeltaSeconds) override;

	int32 GetLoadedChunkCount() const { return Chunks.Num(); }

	/** Server: harvest the tree or boulder at Location. Returns false if it's depleted (regrows later). */
	bool TryHarvest(const FVector& Location);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Chunk edge length in cm (64 m) and vertices per edge minus one (2 m spacing). */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Terrain") float ChunkSize = 6400.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Terrain") int32 Resolution = 32;
	/** Chunks kept around each viewer (radius, in chunks). */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Terrain") int32 ViewRadiusChunks = 5;
	/** Smaller radius around pawns on the server (collision for players and AI). */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Terrain") int32 ServerRadiusChunks = 2;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Terrain") int32 MaxChunkBuildsPerFrame = 2;

	UPROPERTY() TMap<FIntPoint, FNATerrainChunk> Chunks;

	UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
	UPROPERTY() TObjectPtr<UStaticMesh> CylinderShape;
	UPROPERTY() TObjectPtr<UStaticMesh> ConeShape;
	UPROPERTY() TObjectPtr<UStaticMesh> SphereShape;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GrassMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BushMaterial;
	UPROPERTY() TObjectPtr<UStaticMesh> CubeShape;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> SurfaceMaterials;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BarkMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> NeedleMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> StoneMaterial;

private:
	void BuildChunk(const FIntPoint& Key);
	void BuildGround(FNATerrainChunk& Chunk, const FIntPoint& Key);
	void BuildScatter(FNATerrainChunk& Chunk, const FIntPoint& Key);
	void BuildGroundCover(FNATerrainChunk& Chunk, const FIntPoint& Key);
	void DestroyChunk(FNATerrainChunk& Chunk);
	UInstancedStaticMeshComponent* MakeInstances(UStaticMesh* Shape, UMaterialInterface* Material, bool bCollision);
	void GatherViewpoints(TArray<TPair<FVector, int32>>& OutViewpoints) const;

	struct FHarvestState
	{
		int32 HitsLeft = 5;
		float RegrowTime = 0.f;
	};
	TMap<FIntPoint, FHarvestState> HarvestStates;
};
