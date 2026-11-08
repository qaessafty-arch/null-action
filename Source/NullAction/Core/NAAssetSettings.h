#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NAAssetSettings.generated.h"

class USkeletalMesh;
class UStaticMesh;
class UMaterialInterface;
class UAnimInstance;
class USoundBase;
class UNiagaraSystem;
class UNiagaraComponent;
class USceneComponent;

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

/** A real static mesh for a prop (weapon, building piece, crate) and how to fit it. */
USTRUCT(BlueprintType)
struct FNAPropSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Category = "Prop") TSoftObjectPtr<UStaticMesh> Mesh;
	UPROPERTY(EditAnywhere, Config, Category = "Prop") FVector Offset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Config, Category = "Prop") FRotator Rotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, Config, Category = "Prop") float Scale = 1.f;

	bool IsSet() const { return !Mesh.IsNull(); }
};

/** One weapon type: model in the hand plus its sounds. */
USTRUCT(BlueprintType)
struct FNAWeaponAssets
{
	GENERATED_BODY()

	/** Static mesh of the gun/bow. Pivot near the grip; the barrel should point along +X (use Rotation if not). */
	UPROPERTY(EditAnywhere, Config, Category = "Weapon") FNAPropSlot Model;
	UPROPERTY(EditAnywhere, Config, Category = "Weapon") TSoftObjectPtr<USoundBase> FireSound;
	UPROPERTY(EditAnywhere, Config, Category = "Weapon") TSoftObjectPtr<USoundBase> ReloadSound;
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

	// ----- Weapons -----
	UPROPERTY(EditAnywhere, Config, Category = "Weapons") FNAWeaponAssets Pistol;
	UPROPERTY(EditAnywhere, Config, Category = "Weapons") FNAWeaponAssets AssaultRifle;
	UPROPERTY(EditAnywhere, Config, Category = "Weapons") FNAWeaponAssets Shotgun;
	UPROPERTY(EditAnywhere, Config, Category = "Weapons") FNAWeaponAssets SniperRifle;
	UPROPERTY(EditAnywhere, Config, Category = "Weapons") FNAWeaponAssets HuntingBow;

	// ----- Building and props (fit each mesh to the grid size noted) -----
	/** 3 x 3 m floor, 40 cm thick; pivot at the top centre. */
	UPROPERTY(EditAnywhere, Config, Category = "Building") FNAPropSlot Foundation;
	/** 3 m wide, 2.8 m high, along the Y axis; pivot at the bottom centre. */
	UPROPERTY(EditAnywhere, Config, Category = "Building") FNAPropSlot Wall;
	/** Like the wall with a 1.1 x 2.2 m opening in the middle (a working door is added automatically). */
	UPROPERTY(EditAnywhere, Config, Category = "Building") FNAPropSlot Doorway;
	/** ~1 m campfire; pivot at the bottom centre. */
	UPROPERTY(EditAnywhere, Config, Category = "Building") FNAPropSlot Campfire;
	/** Loot crate (~90 x 60 x 55 cm); pivot at the bottom centre. */
	UPROPERTY(EditAnywhere, Config, Category = "Building") FNAPropSlot LootCrate;

	// ----- Sound -----
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> Footstep;
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> BulletImpact;
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> CreatureGrowl;
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> CreatureBite;
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> CreatureDeath;
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> BossRoar;
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> HorseNeigh;
	/** Looping ambience (birds, wind) by day and (insects, owls) by night; crossfades with the clock. */
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> AmbientDay;
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> AmbientNight;
	/** Looping rain/storm bed; volume follows the weather. */
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> RainLoop;
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> MenuMusic;
	UPROPERTY(EditAnywhere, Config, Category = "Sound") TSoftObjectPtr<USoundBase> UIClick;

	// ----- Visual effects (Niagara systems) -----
	UPROPERTY(EditAnywhere, Config, Category = "Effects") TSoftObjectPtr<UNiagaraSystem> MuzzleFlash;
	UPROPERTY(EditAnywhere, Config, Category = "Effects") TSoftObjectPtr<UNiagaraSystem> BulletImpact_FX;
	UPROPERTY(EditAnywhere, Config, Category = "Effects") TSoftObjectPtr<UNiagaraSystem> Blood;
	UPROPERTY(EditAnywhere, Config, Category = "Effects") TSoftObjectPtr<UNiagaraSystem> CampfireFire;
	UPROPERTY(EditAnywhere, Config, Category = "Effects") TSoftObjectPtr<UNiagaraSystem> LightningStrike;
	UPROPERTY(EditAnywhere, Config, Category = "Effects") TSoftObjectPtr<UNiagaraSystem> AshRebirth;
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

	/** Put a prop mesh on a static mesh component (mesh's own materials, fitted). Returns false if the slot is empty/missing. */
	NULLACTION_API bool ApplyProp(const FNAPropSlot& Slot, class UStaticMeshComponent* Component);

	NULLACTION_API USoundBase* LoadSound(const TSoftObjectPtr<USoundBase>& Path);
	/** Plays a configured sound at a location (no-op on a dedicated server or if the slot is empty). */
	NULLACTION_API void PlayAt(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Path, const FVector& Location, float Volume = 1.f);
	/** Spawns a configured Niagara effect (no-op on a dedicated server or if the slot is empty). */
	NULLACTION_API UNiagaraComponent* SpawnEffect(const UObject* WorldContext, const TSoftObjectPtr<UNiagaraSystem>& Path, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
	NULLACTION_API UNiagaraComponent* SpawnEffectAttached(const TSoftObjectPtr<UNiagaraSystem>& Path, USceneComponent* AttachTo, const FVector& RelativeLocation = FVector::ZeroVector);
}
