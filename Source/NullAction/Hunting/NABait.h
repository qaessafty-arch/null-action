#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NABait.generated.h"

class UStaticMeshComponent;
class ANACharacterBase;

/** Meat bait. Roaming creatures within 35 m smell it, walk over and eat for ~10 s (distracted). Lasts 2 minutes. */
UCLASS()
class NULLACTION_API ANABait : public AActor
{
	GENERATED_BODY()

public:
	ANABait();

	/** Server: drop bait on the ground in front of the user. */
	static ANABait* Place(ANACharacterBase* User);

	/** Server: the nearest bait within Radius, or null. */
	static ANABait* FindNearest(UWorld* World, const FVector& Location, float Radius);

	static constexpr float SmellRadius = 3500.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Meat;
};
