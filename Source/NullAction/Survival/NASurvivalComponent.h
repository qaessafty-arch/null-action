#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NASurvivalComponent.generated.h"

/**
 * Server-simulated survival, replicated to the owning player only.
 * - Stamina: sprinting and jumping cost it; at zero you are exhausted.
 * - Hunger / thirst: low values slow stamina recovery; at zero they drain health.
 * - Oxygen (underwater), body temperature (water, cold weather, campfires).
 * - Fatigue: drains while awake; tired players recover stamina slowly. Rest still beside a campfire.
 * - Bleeding (from heavy hits): slow health loss until a Bandage. Untreated for 90 s it becomes an Infection.
 * - Poison (e.g. raw meat): health loss and extra thirst for 40 s, or until an Antidote.
 * - Infection: slow health loss and weak stamina until Medicine.
 * Rates scale with the server's survival difficulty and the player's Survivalist / Endurance skills.
 */
UCLASS(ClassGroup = (NullAction), meta = (BlueprintSpawnableComponent))
class NULLACTION_API UNASurvivalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNASurvivalComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool CanSprint() const { return !bExhausted && Stamina > 5.f; }
	bool HasStamina(float Amount) const { return !bExhausted && Stamina >= Amount; }

	/** Server only. */
	void ConsumeStamina(float Amount);
	float RestoreFood(float Amount);
	float RestoreWater(float Amount);
	void WarmToNormal() { BodyTemperature = NormalTemperature; }
	void StartBleeding();
	bool StopBleeding();
	void Poison(float Seconds);
	bool CurePoison();
	bool CureInfection();
	float Rest(float Amount);
	void SetRebreather(bool bEquipped) { bRebreather = bEquipped; }
	void SetFurCoat(bool bWorn) { bFurCoat = bWorn; }
	bool HasFurCoat() const { return bFurCoat; }

	bool IsBleeding() const { return bBleeding; }
	bool IsPoisoned() const { return PoisonSeconds > 0.f; }
	bool IsInfected() const { return bInfected; }
	bool HasRebreather() const { return bRebreather; }
	float GetFatigueFraction() const { return Fatigue / 100.f; }
	bool IsTired() const { return Fatigue < 25.f; }

	UFUNCTION(BlueprintPure, Category = "NullAction|Survival") float GetStaminaFraction() const { return MaxStamina > 0.f ? Stamina / MaxStamina : 0.f; }
	UFUNCTION(BlueprintPure, Category = "NullAction|Survival") float GetHungerFraction() const { return MaxHunger > 0.f ? Hunger / MaxHunger : 0.f; }
	UFUNCTION(BlueprintPure, Category = "NullAction|Survival") float GetThirstFraction() const { return MaxThirst > 0.f ? Thirst / MaxThirst : 0.f; }
	UFUNCTION(BlueprintPure, Category = "NullAction|Survival") bool IsExhausted() const { return bExhausted; }
	UFUNCTION(BlueprintPure, Category = "NullAction|Survival") float GetOxygenFraction() const { return MaxOxygen > 0.f ? Oxygen / MaxOxygen : 0.f; }
	UFUNCTION(BlueprintPure, Category = "NullAction|Survival") float GetBodyTemperature() const { return BodyTemperature; }
	bool IsHypothermic() const { return BodyTemperature < HypothermiaTemperature; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Stamina") float MaxStamina = 100.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Stamina") float SprintDrainPerSecond = 16.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Stamina") float RegenPerSecond = 20.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Stamina") float RegenDelaySeconds = 1.2f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Stamina") float RecoverFromExhaustionAt = 30.f;

	/** Full to empty in 45 / 30 minutes by default. */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Needs") float MaxHunger = 100.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Needs") float HungerDrainPerSecond = 100.f / (45.f * 60.f);
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Needs") float MaxThirst = 100.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Needs") float ThirstDrainPerSecond = 100.f / (30.f * 60.f);
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Needs") float StarvationDamagePerSecond = 1.f;

	/** Breath held underwater: full to empty in 30 s, then drowning damage. */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Oxygen") float MaxOxygen = 100.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Oxygen") float OxygenDrainPerSecond = 100.f / 30.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Oxygen") float OxygenRecoverPerSecond = 25.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Oxygen") float DrowningDamagePerSecond = 10.f;

	/** Body temperature in °C. Cold water chills you; below the threshold stamina recovers slowly and, colder still, you take damage. */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Temperature") float NormalTemperature = 37.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Temperature") float WaterCoolingPerSecond = 0.08f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Temperature") float WarmingPerSecond = 0.05f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Temperature") float HypothermiaTemperature = 35.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival|Temperature") float SevereHypothermiaTemperature = 33.f;

	UPROPERTY(Replicated) float Stamina = 100.f;
	UPROPERTY(Replicated) float Oxygen = 100.f;
	UPROPERTY(Replicated) float BodyTemperature = 37.f;
	UPROPERTY(Replicated) float Hunger = 100.f;
	UPROPERTY(Replicated) float Thirst = 100.f;
	UPROPERTY(Replicated) bool bExhausted = false;
	UPROPERTY(Replicated) float Fatigue = 100.f; // 100 = fully rested.
	UPROPERTY(Replicated) bool bBleeding = false;
	UPROPERTY(Replicated) float PoisonSeconds = 0.f;
	UPROPERTY(Replicated) bool bInfected = false;
	UPROPERTY(Replicated) bool bRebreather = false;
	UPROPERTY(Replicated) bool bFurCoat = false;

private:
	float LastStaminaUseTime = -100.f;
	float PendingStarvationDamage = 0.f;
	float PendingEnvironmentDamage = 0.f;
	float BleedingStartTime = 0.f;
	float SurvivalScale() const;
	int32 SkillRank(uint8 Skill) const;
};
