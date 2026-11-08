#pragma once

#include "CoreMinimal.h"
#include "AI/NAAITypes.h"
#include "Character/NACharacterBase.h"
#include "NAEnemySoldier.generated.h"

/**
 * Hostile human soldier. Same body, health, hit zones and weapons as players; its brain is
 * ANAEnemyAIController (server only). Replicates its alert state so every player's HUD can warn them.
 */
UCLASS()
class NULLACTION_API ANAEnemySoldier : public ANACharacterBase
{
	GENERATED_BODY()

public:
	ANAEnemySoldier(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Server only. */
	void SetAlert(ENAAlertState NewState, AActor* NewTarget);

	ENAAlertState GetAlertState() const { return AlertState; }
	AActor* GetAlertTarget() const { return AlertTarget; }

	/** World-space patrol route; if empty, the soldier patrols a square around where it spawned. */
	UPROPERTY(EditAnywhere, Category = "NullAction|AI")
	TArray<FVector> PatrolPoints;

	UPROPERTY(EditAnywhere, Category = "NullAction|AI")
	ENADifficulty Difficulty = ENADifficulty::Normal;

protected:
	virtual void BeginPlay() override;
	virtual void OnKilled(AController* Killer) override;

	UPROPERTY(Replicated)
	ENAAlertState AlertState = ENAAlertState::Unaware;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> AlertTarget;
};
