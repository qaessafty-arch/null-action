#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NAAssetSettings.generated.h"

class USkeletalMesh;
class UStaticMesh;
class UMaterialInterface;
class UAnimInstance;

/** A real model for one kind of character/creature: mesh + animation Blueprint + how to fit it. */
USTRUCT(BlueprintType)
struct FNAModelSlot
{
	GENERATED_BODY()

	/** Rigged skeletal mesh (e.g. from Fab / Quixel / Epic samples / your own Blender export). */
	UPROPERTY(EditAnywhere, Config, Category = "Model") TSoftObjectPtr<USkeletalMesh> Mesh;
	/** Animation Blueprint for that skeleton (it should read speed/falling from the pawn, like most packs do). */
	UPROPERTY(EditAnywhere, Config, Category = "Model") TSoftClassPtr<UAnimInstance> AnimClass;
	/** Fit the model to the collision capsule. Most Fab creatures need Z offset = -capsule half height and Yaw -90. */
	UPROPERTY(EditAnywhere, Config, Category = "Model") FVector Offset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Config, Category = "Model") FRotator Rotation = FRotator(0.f, -90.f, 0.f);
	UPROPERTY(EditAnywhere, Config, Category = "Model") float Scale = 1.f;

	bool IsSet() const { return !Mesh.IsNull(); }
};

/**
 * Project Settings -> Game -> NULL-ACTION Assets.
 * Plug real 3D assets into the game without changing code. Every slot is optional: an empty slot keeps the
 * built-in stand-in (engine shapes). Saved to Config/DefaultGame.ini so it's shared through the repository.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "NULL-ACTION Assets"))
class NULLACTION_API UNAAssetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UNAAssetSettings* Get() { return GetDefault<UNAAssetSettings>(); }
	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

	// ----- Characters -----
	UPROPERTY(EditAnywhere, Config, Category = "Characters") FNAModelSlot Player;
	UPROPERTY(EditAnywhere, Config, Category = "Characters") FNAModelSlot Soldier;

	// ----- Creatures & animals -----
	UPROPERTY(EditAnywhere, Config, Category = "Creatures") FNAModelSlot AshStalker;
	UPROPERTY(EditAnywhere, Config, Category = "Creatures") FNAModelSlot Wolf;
	UPROPERTY(EditAnywhere, Config, Category = "Creatures") FNAModelSlot Horse;
	UPROPERTY(EditAnywhere, Config, Category = "Creatures") FNAModelSlot Deer;
	UPROPERTY(EditAnywhere, Config, Category = "Creatures") FNAModelSlot AshGlider;
	UPROPERTY(EditAnywhere, Config, Category = "Creatures") FNAModelSlot Boss;

	// ----- Environment (streamed terrain) -----
	/** Ground material for the procedural terrain (e.g. a Quixel/Fab landscape-style material). Vertex colour = surface type. */
	UPROPERTY(EditAnywhere, Config, Category = "Environment") TSoftObjectPtr<UMaterialInterface> TerrainMaterial;
	/** Tree meshes (one is picked per tree). Pivot at the base; ~10-17 m tall looks right. */
	UPROPERTY(EditAnywhere, Config, Category = "Environment") TArray<TSoftObjectPtr<UStaticMesh>> TreeMeshes;
	UPROPERTY(EditAnywhere, Config, Category = "Environment") TArray<TSoftObjectPtr<UStaticMesh>> RockMeshes;
	UPROPERTY(EditAnywhere, Config, Category = "Environment") TSoftObjectPtr<UStaticMesh> GrassMesh;
	UPROPERTY(EditAnywhere, Config, Category = "Environment") TSoftObjectPtr<UStaticMesh> BushMesh;
	/** Uniform scale for real tree meshes (1 = as authored). */
	UPROPERTY(EditAnywhere, Config, Category = "Environment") float TreeScale = 1.f;
	UPROPERTY(EditAnywhere, Config, Category = "Environment") float RockScale = 1.f;
	UPROPERTY(EditAnywhere, Config, Category = "Environment") float GrassScale = 1.f;
};

namespace NAAssets
{
	/**
	 * Any machine: if the slot is set and the asset exists, put the model on Mesh (anim + fit) and return true.
	 * The caller then hides its stand-in shapes.
	 */
	NULLACTION_API bool ApplyModel(const FNAModelSlot& Slot, class USkeletalMeshComponent* MeshComponent, float CapsuleHalfHeight);

	/** Loads a soft static mesh if it exists in the project, else null. */
	NULLACTION_API UStaticMesh* LoadStatic(const TSoftObjectPtr<UStaticMesh>& Path);
}
