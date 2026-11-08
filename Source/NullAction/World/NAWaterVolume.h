#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NAWaterVolume.generated.h"

class UStaticMeshComponent;

/**
 * A body of water characters can swim and dive in. The actor's location is the centre of the water;
 * Extent is half the size. Characters ask it for the surface height; swimming, oxygen and cold
 * are handled by the character and survival component.
 */
UCLASS()
class NULLACTION_API ANAWaterVolume : public AActor
{
	GENERATED_BODY()

public:
	ANAWaterVolume();

	/** True if Point is inside (or just above) this water; returns the surface height. */
	bool GetSurfaceAt(const FVector& Point, float& OutSurfaceZ) const;

	/** Finds the water (if any) at a point in the world. */
	static bool FindWaterAt(const UWorld* World, const FVector& Point, float& OutSurfaceZ);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "NullAction|Water")
	FVector Extent = FVector(660.f, 660.f, 140.f);

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Water")
	TObjectPtr<UStaticMeshComponent> Surface;
};
