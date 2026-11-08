#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NAHuntTrap.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ANACharacterBase;

/**
 * Snare trap: a low, dark jaw trap set by a player. Snaps shut on the first creature or enemy that steps
 * in it (30 damage). Creatures and AI soldiers are held in place for 5 s; players only take the damage
 * (player movement is client-predicted, so it isn't frozen from the server). Single use. Never triggers
 * on the player who set it. Max 5 armed traps per player. Server-authoritative.
 */
UCLASS()
class NULLACTION_API ANAHuntTrap : public AActor
{
	GENERATED_BODY()

public:
	ANAHuntTrap();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: set a trap on the ground in front of the user. Returns null if there's no room. */
	static ANAHuntTrap* Place(ANACharacterBase* User);

	static constexpr int32 MaxTrapsPerPlayer = 5;
	static constexpr float Damage = 30.f;
	static constexpr float HoldSeconds = 5.f;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Trigger;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Plate;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> HingeA;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> HingeB;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> JawA;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> JawB;

	UPROPERTY(ReplicatedUsing = OnRep_Sprung) bool bSprung = false;

	UFUNCTION() void OnRep_Sprung();
	UFUNCTION() void OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	virtual void BeginPlay() override;

private:
	void Spring(AActor* Victim);
	TWeakObjectPtr<APawn> Placer;
};
