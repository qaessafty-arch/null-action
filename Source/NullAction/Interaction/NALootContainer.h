#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NAInteractable.h"
#include "NALootContainer.generated.h"

class UStaticMeshComponent;

/**
 * A crate that gives random supplies once, then refills after a while. The server rolls the loot:
 * ammo, one supply, and a bonus item by rarity tier (Common 55%, Uncommon 30%, Rare 12%, Epic 3%).
 * High-quality crates (supply drops, the shipwreck) always give a Rare-or-better item and an Ember Cell.
 */
UCLASS()
class NULLACTION_API ANALootContainer : public AActor, public INAInteractable
{
	GENERATED_BODY()

public:
	ANALootContainer();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual bool CanInteract(const ANACharacterBase* User) const override { return !bOpened; }
	virtual void Interact(ANACharacterBase* User) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "NullAction|Loot") float RefillSeconds = 300.f;
public:
	/** Server, before FinishSpawning. */
	UPROPERTY(EditAnywhere, Category = "NullAction|Loot") bool bHighQuality = false;
	/** World-event crate: never refills; tells the game mode who opened it. */
	UPROPERTY(EditAnywhere, Category = "NullAction|Loot") bool bEventCrate = false;
protected:

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Loot") TObjectPtr<UStaticMeshComponent> Box;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Loot") TObjectPtr<UStaticMeshComponent> Lid;

	UPROPERTY(ReplicatedUsing = OnRep_Opened) bool bOpened = false;

	UFUNCTION() void OnRep_Opened();

private:
	void Refill();
	FTimerHandle RefillTimer;
};
