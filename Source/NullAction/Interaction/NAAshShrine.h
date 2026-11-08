#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NAInteractable.h"
#include "NAAshShrine.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/**
 * Ash Shrine: the heart of ASH REBIRTH.
 *  - Use it to attune: when you die with an Ash charge left, you are reborn here (keeping your items).
 *  - Offer an Ember Fang (from Ash Stalkers) to gain an extra charge (up to 3).
 * Rebirth costs a charge, has a 5-minute cooldown, and leaves you Ash-touched (slower) for a while.
 */
UCLASS()
class NULLACTION_API ANAAshShrine : public AActor, public INAInteractable
{
	GENERATED_BODY()

public:
	ANAAshShrine();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual bool CanInteract(const ANACharacterBase* User) const override;
	virtual void Interact(ANACharacterBase* User) override;

	/** Server only; set before FinishSpawning. Unique, shown to players. */
	void SetShrineName(const FString& NewName) { ShrineName = NewName; }
	const FString& GetShrineName() const { return ShrineName; }

	/** Where a reborn player appears. */
	FTransform GetRebirthTransform() const;

	/** Server: play the rebirth flare for everyone. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRebirthFlare();

	static FName EmberFangId() { return FName(TEXT("EmberFang")); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Base;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Pillar;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Ember;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY(Replicated) FString ShrineName = TEXT("Ash Shrine");

private:
	float FlareUntil = -1.f;
};
