#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NAInteractable.h"
#include "NAResourceNode.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ENAResourceNodeType : uint8
{
	HerbBush,
	BerryBush,
	ScrapPile
};

/** Something you gather by hand (E). Depletes, then regrows. The server decides what you get. */
UCLASS()
class NULLACTION_API ANAResourceNode : public AActor, public INAInteractable
{
	GENERATED_BODY()

public:
	ANAResourceNode();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual bool CanInteract(const ANACharacterBase* User) const override { return bAvailable; }
	virtual void Interact(ANACharacterBase* User) override;

	/** Set before FinishSpawning. */
	UPROPERTY(EditAnywhere, Replicated, Category = "NullAction|Resource") ENAResourceNodeType NodeType = ENAResourceNodeType::HerbBush;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "NullAction|Resource") float RegrowSeconds = 90.f;
	UPROPERTY(ReplicatedUsing = OnRep_Available) bool bAvailable = true;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	UFUNCTION() void OnRep_Available();

private:
	void Regrow();
	FTimerHandle RegrowTimer;
};

/** A dropped stack of items (creature parts, salvage). Picked up with E; disappears after a while. */
UCLASS()
class NULLACTION_API ANAItemPickup : public AActor, public INAInteractable
{
	GENERATED_BODY()

public:
	ANAItemPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual void Interact(ANACharacterBase* User) override;

	/** Server: drop items at a location. */
	static ANAItemPickup* Drop(UWorld* World, const FVector& Location, FName DroppedItem, int32 DroppedCount);

	UPROPERTY(Replicated) FName ItemId;
	UPROPERTY(Replicated) int32 Count = 1;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Pickup") TObjectPtr<UStaticMeshComponent> Mesh;
};
