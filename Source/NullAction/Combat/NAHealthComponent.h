#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/NACombatTypes.h"
#include "NAHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNAOnHealthChanged, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNAOnDeathStateChanged, bool, bIsDead);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNAOnKilled, AActor*, Victim, AController*, Killer);

/**
 * Replicated health and armor. Only the server changes values; clients get them by replication.
 * Armor absorbs part of body/limb damage (not headshots) until it is used up.
 */
UCLASS(ClassGroup = (NullAction), meta = (BlueprintSpawnableComponent))
class NULLACTION_API UNAHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNAHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server only. Returns the health actually removed. */
	float ApplyDamage(float Amount, ENAHitZone Zone, AController* Instigator, AActor* Causer, bool bBypassArmor = false);

	/** Server only. Returns the health actually restored. */
	float Heal(float Amount);

	/** Server only. Returns the armor actually added. */
	float AddArmor(float Amount);

	/** Server only: when set, no damage of any kind is applied (owner God mode). */
	void SetInvulnerable(bool bNewInvulnerable) { bInvulnerable = bNewInvulnerable; }
	bool IsInvulnerable() const { return bInvulnerable; }

	/** Set before play starts (e.g. in an owner's constructor) to change toughness. */
	void SetMaxHealth(float NewMaxHealth) { MaxHealth = FMath::Max(1.f, NewMaxHealth); Health = MaxHealth; }

	/** Server only. Brings a dead owner back with a fraction of max health (respawns, dummies, future revive). */
	void Revive(float HealthFraction = 1.f);

	UFUNCTION(BlueprintPure, Category = "NullAction|Health") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "NullAction|Health") float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "NullAction|Health") float GetArmor() const { return Armor; }
	UFUNCTION(BlueprintPure, Category = "NullAction|Health") float GetMaxArmor() const { return MaxArmor; }
	UFUNCTION(BlueprintPure, Category = "NullAction|Health") bool IsDead() const { return bDead; }

	/** Local world time of the last health loss, for HUD damage feedback. */
	float GetLastDamageTime() const { return LastDamageTime; }

	UPROPERTY(BlueprintAssignable, Category = "NullAction|Health")
	FNAOnHealthChanged OnHealthChanged;

	/** Fires on every machine when the owner dies or is revived. */
	UPROPERTY(BlueprintAssignable, Category = "NullAction|Health")
	FNAOnDeathStateChanged OnDeathStateChanged;

	/** Server only: who killed whom, for game rules. */
	UPROPERTY(BlueprintAssignable, Category = "NullAction|Health")
	FNAOnKilled OnKilled;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Health")
	float MaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Health")
	float MaxArmor = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Health")
	float StartingArmor = 0.f;

	/** Share of incoming body/limb damage taken by armor while armor lasts. */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Health", meta = (ClampMin = 0, ClampMax = 1))
	float ArmorAbsorption = 0.6f;

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float Health = 100.f;

	UPROPERTY(Replicated)
	float Armor = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_Dead)
	bool bDead = false;

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION()
	void OnRep_Dead();

private:
	void NotifyHealthChanged(float OldHealth);

	float LastDamageTime = -100.f;
	bool bInvulnerable = false;
};
