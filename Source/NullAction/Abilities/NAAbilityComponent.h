#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/NAPlayerState.h"
#include "NAAbilityComponent.generated.h"

/**
 * One power per class, on Q. Energy and cooldowns are owned by the server; the client only asks.
 *  PHANTOM  - Cloak: invisible to players and AI for 8 s (footsteps still heard; shooting or getting close reveals you).
 *  HUNTER   - Tracking Pulse: see every enemy and creature within 80 m through walls for 10 s.
 *  STORM    - Lightning Strike: after a 0.6 s warning, 70 damage in a 3.5 m radius where you aim (up to 40 m).
 *  GUARDIAN - Restoration Field: 8 s field that heals you and nearby teammates; revives go faster inside it.
 * Each has visible counterplay (warning ring, glow, sound cues) so it stays fair in PvP.
 */
UCLASS(ClassGroup = (NullAction), meta = (BlueprintSpawnableComponent))
class NULLACTION_API UNAAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNAAbilityComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Owning client or server: try to use the class power. */
	void RequestActivate();

	ENAArchetype GetArchetype() const;
	static FString AbilityName(ENAArchetype Archetype);
	float GetEnergyFraction() const { return Energy / MaxEnergy; }
	float GetCooldownRemaining() const;
	float GetActiveRemaining() const;
	bool IsCloaked() const;
	bool IsTrackingActive() const;
	bool IsFieldActive() const;
	FVector GetStrikeLocation() const { return StrikeLocation; }
	float GetStrikeWarningEnd() const { return StrikeWarningEnd; }

	/** Server: firing or taking damage while cloaked breaks the cloak. */
	void BreakCloak();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Ability") float MaxEnergy = 100.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Ability") float EnergyRegenPerSecond = 6.f;

	UPROPERTY(Replicated) float Energy = 100.f;
	/** Server time values, replicated so every client can show effects in sync. */
	UPROPERTY(Replicated) float CooldownEnd = 0.f;
	UPROPERTY(Replicated) float ActiveEnd = 0.f;
	UPROPERTY(Replicated) FVector_NetQuantize StrikeLocation;
	UPROPERTY(Replicated) float StrikeWarningEnd = 0.f;

	UFUNCTION(Server, Reliable)
	void ServerActivate(FVector_NetQuantize AimPoint);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastLightning(FVector_NetQuantize Location);

private:
	void Activate(const FVector& AimPoint);
	void ResolveStrike();
	float ServerTime() const;
	FVector GetAimPoint() const;

	bool bStrikePending = false;
	float NextFieldPulse = 0.f;
};
