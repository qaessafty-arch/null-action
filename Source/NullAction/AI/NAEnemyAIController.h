#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/NAAITypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "NAEnemyAIController.generated.h"

class ANAEnemySoldier;
class ANAWeapon;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

UENUM()
enum class ENASoldierState : uint8
{
	Patrol,
	Investigate,
	Combat,
	TakeCover,
	Search,
	Retreat
};

/**
 * Server-side brain for ANAEnemySoldier: a state machine driven by AI Perception (sight + hearing).
 *
 *   Patrol -> (half-noticed / heard a noise) -> Investigate -> Patrol
 *   Patrol/Investigate -> (fully noticed) -> Combat <-> TakeCover (reload, relocate)
 *   Combat -> (lost sight) -> Search -> Patrol      Combat -> (badly hurt) -> Retreat -> Combat
 *
 * Difficulty (FNADifficultyProfile) changes reaction time, aim, cover use, flanking, retreating,
 * investigating and calling allies.
 */
UCLASS()
class NULLACTION_API ANAEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANAEnemyAIController();

	void SetDifficulty(ENADifficulty NewDifficulty);

	/** Another soldier reports a target. */
	void ReceiveAlert(AActor* AlertedTarget, const FVector& Location);

	ENASoldierState GetSoldierState() const { return State; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|AI")
	TObjectPtr<UAIPerceptionComponent> Perception;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	void Think();
	void UpdatePerception(float DeltaSeconds, float Now);
	void EnterState(ENASoldierState NewState);
	void EnterCombat();

	void TickPatrol(float Now);
	void TickInvestigate(float Now);
	void TickCombat(float Now);
	void TickTakeCover(float Now);
	void TickSearch(float Now);
	void TickRetreat(float Now);

	void MoveToPoint(const FVector& Destination, float AcceptanceRadius);
	bool IsMoveIdle() const;
	bool ProjectToGround(const FVector& Point, FVector& OutGround) const;
	bool IsStandable(const FVector& Ground) const;
	bool FindCover(const FVector& ThreatEye, bool bRetreat, FVector& OutCover) const;
	void AimAtTarget();
	void UpdateBurst(float Now);
	void StopFiring();
	ANAEnemySoldier* GetSoldier() const;
	ANAWeapon* GetWeapon() const;

	FNADifficultyProfile Profile;
	ENASoldierState State = ENASoldierState::Patrol;
	float StateStartTime = 0.f;
	float LastThinkTime = 0.f;

	TWeakObjectPtr<AActor> Target;
	bool bTargetVisible = false;
	FVector LastKnownLocation = FVector::ZeroVector;
	float LastSeenTime = -100.f;
	float Awareness = 0.f;

	FVector InvestigateLocation = FVector::ZeroVector;
	FVector CoverLocation = FVector::ZeroVector;
	FVector HomeLocation = FVector::ZeroVector;
	TArray<FVector> Route;
	int32 RouteIndex = 0;
	float WaitUntil = 0.f;

	bool bFiring = false;
	float BurstEndTime = 0.f;
	float NextBurstTime = 0.f;
	float NextRepositionTime = 0.f;
	FVector AimOffset = FVector::ZeroVector;
	bool bAimForHead = false;
	bool bHasRetreated = false;

	FTimerHandle ThinkTimer;
};
