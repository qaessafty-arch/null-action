#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NAInteractable.h"
#include "NACreatureTrack.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * A pair of paw prints a creature leaves as it moves (one every ~4.5 m; they fade after 90 s).
 * Look at one to read how fresh it is and which way the animal was heading. Hunters on a Tracking
 * Pulse see nearby tracks highlighted.
 */
UCLASS()
class NULLACTION_API ANACreatureTrack : public AActor, public INAInteractable
{
	GENERATED_BODY()

public:
	ANACreatureTrack();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual void Interact(ANACharacterBase* User) override;

	/** Server. */
	static ANACreatureTrack* Spawn(UWorld* World, const FVector& GroundLocation, float Yaw, const FString& Name);

	float GetAgeSeconds() const;
	static FString CompassName(float Yaw);

	static constexpr float LifeSeconds = 90.f;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Hitbox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PrintLeft;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PrintRight;

	UPROPERTY(Replicated) float SpawnServerTime = 0.f;
	UPROPERTY(Replicated) FString CreatureName;

	virtual void BeginPlay() override;
};
