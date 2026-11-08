#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "NACreatureAIController.generated.h"

class ANACreature;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

UENUM()
enum class ENACreatureState : uint8
{
	Roam,
	Stalk,
	Hunt,
	Flee,
	Return
};

/**
 * Predator brain for the Ash Stalker (server only). It does not behave like a soldier:
 *  - Roams its territory around a den, pausing to sniff around.
 *  - Stalks: circles prey low and slow at a distance, waiting for an opening.
 *  - Ambushes: charges when the prey looks away, or after stalking long enough.
 *  - Hit-and-run: bites, springs back, charges again. Sidesteps when the prey aims at it.
 *  - Drawn to gunshots; answers an attack from an unseen shooter.
 *  - Flees to its den when badly hurt and recovers there; abandons prey that leaves its territory.
 */
UCLASS()
class NULLACTION_API ANACreatureAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANACreatureAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|AI")
	TObjectPtr<UAIPerceptionComponent> Perception;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|AI") float TerritoryRadius = 4500.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|AI") float RoamRadius = 1500.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|AI") float StalkDistance = 900.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|AI") float FleeHealthFraction = 0.3f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|AI") float RecoverHealthFraction = 0.7f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|AI") float DenHealPerSecond = 6.f;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	void Think();
	void UpdatePrey(float Now);
	void EnterState(ENACreatureState NewState);
	void TickRoam(float Now);
	void TickStalk(float Now, float DeltaSeconds);
	void TickHunt(float Now);
	void TickFlee(float Now, float DeltaSeconds);
	void TickReturn(float Now);
	void MoveToPoint(const FVector& Destination, float AcceptanceRadius);
	bool IsMoveIdle() const;
	bool IsPreyLookingAtMe(float MinDot) const;
	ANACreature* GetCreature() const;

	ENACreatureState State = ENACreatureState::Roam;
	float StateStartTime = 0.f;
	float LastThinkTime = 0.f;
	float WaitUntil = 0.f;

	TWeakObjectPtr<AActor> Prey;
	bool bPreyVisible = false;
	FVector LastKnownLocation = FVector::ZeroVector;
	float LastSeenTime = -100.f;

	float OrbitAngle = 0.f;
	float OrbitDirection = 1.f;
	float NextDodgeTime = 0.f;
	float RecoilUntil = 0.f;
	bool bHasFled = false;

	FTimerHandle ThinkTimer;
};
