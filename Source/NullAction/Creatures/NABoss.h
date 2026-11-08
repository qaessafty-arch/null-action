#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NABoss.generated.h"

class UNAHealthComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UPointLightComponent;

UENUM(BlueprintType)
enum class ENABossState : uint8
{
	Dormant,
	Awake,
	Dead
};

/**
 * ASHEN BEHEMOTH: the first boss. A huge armored ash beast sleeping in the Ash Crater.
 *  - Wakes when a player comes close (or shoots it). Health scales with the number of players.
 *  - Armored hide: body hits do 50%, legs 70%. The glowing EMBER CORE on its back and its head are weak points.
 *  - Phase 1: Ground Slam (red circle telegraph, then a shockwave) and Charge.
 *  - Phase 2 (below 60%): adds Ember Rain, three burning circles around the target.
 *  - Phase 3 (below 25%): ENRAGED, faster and shorter warnings.
 *  - Leash: if everyone flees or dies, it walks home and heals.
 * Everything is decided on the server; warnings, phase and health replicate so every player sees the same fight.
 * Built from engine shapes (original design, no third-party assets) until a sculpted model exists.
 */
UCLASS()
class NULLACTION_API ANABoss : public ACharacter
{
	GENERATED_BODY()

public:
	ANABoss(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UNAHealthComponent* GetHealthComponent() const { return HealthComponent; }
	ENABossState GetBossState() const { return BossState; }
	int32 GetPhase() const { return Phase; }
	bool IsDead() const;
	static FString BossName() { return TEXT("Ashen Behemoth"); }

	/** Active telegraph circles (any machine): location, radius and server time when they hit. */
	int32 NumWarnings() const { return WarningLocations.Num(); }
	FVector GetWarningLocation(int32 Index) const { return WarningLocations[Index]; }
	float GetWarningRadius(int32 Index) const { return WarningRadii[Index]; }
	float GetWarningEnd(int32 Index) const { return WarningEnds[Index]; }

	/** Server: where it sleeps and returns to. */
	void SetLairLocation(const FVector& Location) { LairLocation = Location; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UNAHealthComponent> HealthComponent;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> VisualRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> HeadJoint;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> CoreGlow;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Hips;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> AshParts;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> EmberParts;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> HitParts;
	/** Pooled red circles for the telegraphs (cosmetic, positioned in world space). */
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> WarningDiscs;

	UPROPERTY(ReplicatedUsing = OnRep_BossState) ENABossState BossState = ENABossState::Dormant;
	UPROPERTY(Replicated) uint8 Phase = 1;
	UPROPERTY(Replicated) TArray<FVector_NetQuantize> WarningLocations;
	UPROPERTY(Replicated) TArray<float> WarningRadii;
	UPROPERTY(Replicated) TArray<float> WarningEnds;
	UPROPERTY(ReplicatedUsing = OnRep_SlamCount) uint8 SlamCount = 0;

	UFUNCTION() void OnRep_BossState();
	UFUNCTION() void OnRep_SlamCount();
	UFUNCTION() void HandleKilled(AActor* Victim, AController* Killer);
	UFUNCTION() void HandleDeathStateChanged(bool bNowDead);

private:
	UStaticMeshComponent* MakePart(FName PartName, USceneComponent* Parent, UStaticMesh* Shape, const FVector& Location,
		const FVector& SizeCm, const FRotator& Rotation, bool bEmber, FName HitTag = NAME_None);

	// Server brain.
	void ThinkServer(float DeltaSeconds);
	void Wake(AActor* WokenBy);
	void ResetToLair();
	APawn* FindTarget(float Radius) const;
	void StartSlam();
	void StartEmberRain(const APawn* Victim);
	void StartCharge(const APawn* Victim);
	void AddWarning(const FVector& Location, float Radius, float Delay, float Damage);
	void ResolveWarnings();
	float ServerNow() const;
	float PhaseSpeed() const;

	// Cosmetic.
	void AnimateBody(float DeltaSeconds);
	void UpdateWarningDiscs();

	struct FPendingStrike { FVector Location; float Radius; float Time; float Damage; };
	TArray<FPendingStrike> PendingStrikes;

	FVector LairLocation = FVector::ZeroVector;
	TWeakObjectPtr<APawn> CurrentTarget;
	float NextAttackTime = 0.f;
	float BusyUntil = 0.f;
	float ChargeUntil = 0.f;
	FVector ChargeDirection = FVector::ForwardVector;
	bool bChargeHit = false;
	float LastTargetSeenTime = 0.f;
	float GaitPhase = 0.f;
	float SlamAnimStart = -100.f;
	ENABossState LastSeenState = ENABossState::Dormant;
};
