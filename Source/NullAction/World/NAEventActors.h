#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NAInteractable.h"
#include "NAEventActors.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/** Ember Storm strike: a red circle on the ground, then a burst of fire 1.6 s later (server damage). */
UCLASS()
class NULLACTION_API ANAEmberStrike : public AActor
{
	GENERATED_BODY()

public:
	ANAEmberStrike();
	virtual void Tick(float DeltaSeconds) override;
	static constexpr float Radius = 320.f;
	static constexpr float Delay = 1.6f;
	static constexpr float Damage = 30.f;

protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Disc;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;

private:
	float Age = 0.f;
	bool bBurst = false;
};

/**
 * Distress beacon (world event). E switches it on; it transmits for 45 s and draws enemy soldiers.
 * If a player is still within 20 m when it finishes, the event succeeds.
 */
UCLASS()
class NULLACTION_API ANADistressBeacon : public AActor, public INAInteractable
{
	GENERATED_BODY()

public:
	ANADistressBeacon();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual bool CanInteract(const ANACharacterBase* User) const override { return !bActive && !bDone; }
	virtual void Interact(ANACharacterBase* User) override;

	bool IsActive() const { return bActive; }
	float GetTransmitRemaining() const;

	static constexpr float TransmitSeconds = 45.f;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mast;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Lamp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;
	UPROPERTY(Replicated) bool bActive = false;
	UPROPERTY(Replicated) bool bDone = false;
	UPROPERTY(Replicated) float TransmitEndServerTime = 0.f;
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<AController> Activator;
};

/** Sunken wreck at the bottom of Mirror Lake: a hull, a broken mast, and a high-quality crate inside. */
UCLASS()
class NULLACTION_API ANAShipwreck : public AActor
{
	GENERATED_BODY()

public:
	ANAShipwreck();
	/** Where the game mode puts the crate. */
	FTransform CrateTransform() const;

protected:
	virtual void BeginPlay() override;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Planks;
};
