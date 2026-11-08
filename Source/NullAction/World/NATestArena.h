#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NATestArena.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;

/**
 * Self-contained movement test course built entirely from engine shapes, so the project
 * is playable without any imported content. Contains: a large grid floor, stairs, a ramp,
 * jump boxes of increasing height, a crouch-only tunnel, pillars for camera-collision checks,
 * and its own sun/sky/fog (skipped automatically if the level already has a sun).
 *
 * Spawned automatically by ANAGameMode in levels without PlayerStarts; can also be placed by hand.
 * Replicated so clients joining a server get identical collision and visuals.
 */
UCLASS()
class NULLACTION_API ANATestArena : public AActor
{
	GENERATED_BODY()

public:
	ANATestArena();

	/** World-space spawn transform for player slot Index (players are spread in a ring). */
	FTransform GetSpawnTransform(int32 Index) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "NullAction|Arena")
	bool bProvideLighting = true;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Arena")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Arena")
	TArray<TObjectPtr<UStaticMeshComponent>> Blocks;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Arena|Lighting")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Arena|Lighting")
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Arena|Lighting")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Arena|Lighting")
	TObjectPtr<UExponentialHeightFogComponent> Fog;

private:
	/** Adds a box. Location is the box's bottom-centre; Size is in centimetres. */
	UStaticMeshComponent* AddBox(FName Name, const FVector& BottomCentre, const FVector& Size, const FRotator& Rotation, int32 ColorIndex);

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GridMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ShapeMaterial;

	/** Per-block colour index (-1 = grid floor), parallel to Blocks. */
	UPROPERTY()
	TArray<int32> BlockColors;
};
