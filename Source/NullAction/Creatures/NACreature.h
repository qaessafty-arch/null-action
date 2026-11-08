#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/NAInteractable.h"
#include "NACreature.generated.h"

class UNAHealthComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UPointLightComponent;
class USoundBase;

UENUM(BlueprintType)
enum class ENACreatureMood : uint8
{
	Roaming,
	Stalking,
	Hunting,
	Fleeing
};

/**
 * ASH STALKER: an original NULL-ACTION predator. A low, long-bodied hunter with ember-glowing back
 * spines and burning eyes. Its eyes/head are the weak point (head-shot multiplier applies).
 *
 * The body is assembled from engine shapes and animated procedurally in code (trotting legs, tail,
 * breathing, head tracking, bite lunge, death collapse) until a sculpted, rigged model is imported.
 * Brain: ANACreatureAIController (server). Mood and bites replicate so every player sees the same hunt.
 */
UCLASS()
class NULLACTION_API ANACreature : public ACharacter, public INAInteractable
{
	GENERATED_BODY()

public:
	ANACreature(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UNAHealthComponent* GetHealthComponent() const { return HealthComponent; }
	bool IsDead() const;
	ENACreatureMood GetMood() const { return Mood; }
	AActor* GetPrey() const { return Prey; }
	float GetBiteRange() const { return BiteRange; }
	const FVector& GetDenLocation() const { return DenLocation; }

	/** Server only. */
	void SetMood(ENACreatureMood NewMood, AActor* NewPrey);
	void SetMoveSpeed(float Speed);
	bool TryBite(AActor* Victim);
	void SetDenLocation(const FVector& Location) { DenLocation = Location; }

	// ----- Hunting: skin the carcass with a Hunting Knife (E) -----
	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual bool CanInteract(const ANACharacterBase* User) const override;
	virtual void Interact(ANACharacterBase* User) override;

	// ----- Species (subclasses such as ANAWolf override these) -----
	virtual FString GetSpeciesName() const { return TEXT("Ash Stalker"); }
	virtual int32 GetXPReward() const { return 60; }
	/** What respawns it when it dies: 0 = Ash Stalker den, 1 = wolf pack, 2 = never (world-event creature). */
	uint8 GetSpawnGroup() const { return SpawnGroup; }
	void SetSpawnGroup(uint8 Group) { SpawnGroup = Group; }

	/** Last player who hurt it (server), so the AI can answer an attack it didn't see coming. */
	AActor* GetLastAttacker() const { return LastAttacker.Get(); }
	void ClearLastAttacker() { LastAttacker = nullptr; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Creature")
	TObjectPtr<UNAHealthComponent> HealthComponent;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Creature") float BiteDamage = 25.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Creature") float BiteRange = 230.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Creature") float BiteCooldownSeconds = 1.6f;

	/** Optional audio, played if assigned in a Blueprint subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Creature|Audio") TObjectPtr<USoundBase> GrowlSound;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Creature|Audio") TObjectPtr<USoundBase> BiteSound;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Creature|Audio") TObjectPtr<USoundBase> DeathSound;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Creature|Body") TObjectPtr<USceneComponent> VisualRoot;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Creature|Body") TObjectPtr<USceneComponent> NeckJoint;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Creature|Body") TObjectPtr<USceneComponent> JawJoint;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Creature|Body") TObjectPtr<UPointLightComponent> EyeGlow;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Hips;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Knees;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> TailJoints;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> AshParts;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> EmberParts;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> HitParts;

	UPROPERTY(Replicated) ENACreatureMood Mood = ENACreatureMood::Roaming;
	UPROPERTY(Replicated) TObjectPtr<AActor> Prey;
	UPROPERTY(ReplicatedUsing = OnRep_BiteCount) uint8 BiteCount = 0;
	UPROPERTY(Replicated) bool bSkinned = false;

	/** Body colours applied in BeginPlay (dark ash and glowing embers by default). */
	FLinearColor BodyColor = FLinearColor(0.055f, 0.05f, 0.05f);
	FLinearColor GlowColor = FLinearColor(1.f, 0.35f, 0.04f);
	uint8 SpawnGroup = 0;

	/** Server: what it drops on death, and what skinning gives. */
	virtual void DropLoot();
	virtual FName GetSkinItem() const;

	UFUNCTION() void OnRep_BiteCount();
	UFUNCTION() void HandleDeathStateChanged(bool bNowDead);
	UFUNCTION() void HandleKilled(AActor* Victim, AController* Killer);

private:
	USceneComponent* MakeJoint(FName JointName, USceneComponent* Parent, const FVector& Location);
	UStaticMeshComponent* MakePart(FName PartName, USceneComponent* Parent, UStaticMesh* Shape, const FVector& Location,
		const FVector& SizeCm, const FRotator& Rotation, bool bEmber, FName HitTag = NAME_None);
	void AnimateBody(float DeltaSeconds);

	FVector DenLocation = FVector::ZeroVector;
	TWeakObjectPtr<AActor> LastAttacker;
	float GaitPhase = 0.f;
	float LifeTime = 0.f;
	float BiteAnimStartTime = -100.f;
	float LastBiteTime = -100.f;
	FVector LastTrackLocation = FVector::ZeroVector;
	bool bLeftFirstTrack = false;
	bool bTrackLeft = false;
};
