#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NACabin.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UPointLightComponent;

/**
 * THE OLD CABIN: a walk-in building with three furnished rooms (living room, bedroom, storage),
 * doorways, windows, a roof and warm interior lights. Doors, the loot crate and the note are separate
 * interactive actors placed by the game mode at the positions this class reports.
 * Local space: origin = centre of the floor; +X = front (door side).
 */
UCLASS()
class NULLACTION_API ANACabin : public AActor
{
	GENERATED_BODY()

public:
	ANACabin();

	/** Where interactive pieces go (local -> world). */
	FTransform FrontDoorTransform() const;
	FTransform StorageDoorTransform() const;
	FTransform CrateTransform() const;
	FTransform NoteTransform() const;
	FTransform MedkitTransform() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY() TArray<FLinearColor> PartColors;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Cabin") TObjectPtr<UPointLightComponent> LivingLight;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Cabin") TObjectPtr<UPointLightComponent> BedroomLight;

private:
	void AddBox(const TCHAR* PartName, const FVector& BottomCentre, const FVector& Size, const FLinearColor& Color, float Yaw = 0.f);

	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> ShapeMaterial;
};
