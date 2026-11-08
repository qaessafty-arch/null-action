#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NACharacterBase.generated.h"

class UStaticMeshComponent;
class USkeletalMesh;
class UAnimInstance;
class UNAHealthComponent;
class ANAWeapon;

/** Locomotion gait. Crouching is handled separately by ACharacter's built-in, predicted crouch. */
UENUM(BlueprintType)
enum class ENAGait : uint8
{
	Walk,
	Run,
	Sprint
};

/**
 * Shared humanoid base for every NULL-ACTION character: players, enemy bots, NPCs, training dummies.
 * Owns locomotion, health and death, the weapon inventory, aiming and body appearance.
 * Player-only concerns (camera, input, survival needs, interaction) live in ANAPlayerCharacter.
 */
UCLASS(Abstract)
class NULLACTION_API ANACharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ANACharacterBase(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Destroyed() override;
	virtual void Landed(const FHitResult& Hit) override;

	// ----- Movement -----

	/** Request a gait. Applied locally right away and sent to the server, which owns the replicated value. */
	UFUNCTION(BlueprintCallable, Category = "NullAction|Movement")
	void SetDesiredGait(ENAGait NewGait);

	UFUNCTION(BlueprintPure, Category = "NullAction|Movement")
	ENAGait GetGait() const { return Gait; }

	UFUNCTION(BlueprintPure, Category = "NullAction|Movement")
	bool IsSprinting() const { return Gait == ENAGait::Sprint && !bIsCrouched; }

	UFUNCTION(BlueprintCallable, Category = "NullAction|Movement")
	void ToggleCrouch();

	/** Players override this with stamina rules. */
	virtual bool CanSprint() const { return true; }

	/** Prone: lying flat. Slowest, quietest, steadiest aim, hardest to spot. Any machine; replicated by the server. */
	void SetProne(bool bNewProne);

	UFUNCTION(BlueprintPure, Category = "NullAction|Movement")
	bool IsProne() const { return bIsProne; }

	/** Overridden by characters that can swim. */
	virtual bool IsInWater() const { return false; }
	virtual bool IsHeadUnderwater() const { return false; }

	/** Server only: ignore all damage for a moment (dodge roll). */
	void GrantInvulnerability(float Seconds);

	// ----- Health -----

	UFUNCTION(BlueprintPure, Category = "NullAction|Health")
	UNAHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "NullAction|Health")
	bool IsDead() const;

	// ----- Weapons -----

	/** Server only. Gives a weapon (or ammo, if already owned). Returns nullptr if nothing was gained. */
	ANAWeapon* GiveWeapon(TSubclassOf<ANAWeapon> WeaponClass, bool bEquip);

	/** Any machine: the owning client asks the server. */
	void EquipWeapon(ANAWeapon* Weapon);
	void EquipWeaponInSlot(int32 Slot);
	void CycleWeapon(int32 Direction);

	UFUNCTION(BlueprintPure, Category = "NullAction|Weapons")
	ANAWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	const TArray<TObjectPtr<ANAWeapon>>& GetWeapons() const { return Weapons; }

	void SetAiming(bool bNewAiming);

	UFUNCTION(BlueprintPure, Category = "NullAction|Weapons")
	bool IsAiming() const { return bIsAiming; }

	/** Where shots originate and which way they go: the camera for players, the eyes for AI. */
	virtual void GetAimViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	/** Shooter's machine only: kick the view after a shot. */
	virtual void ApplyRecoil(float PitchDegrees, float YawDegrees) {}

	/** Shooter's machine only: the server confirmed one of our shots hit. */
	void NotifyHitConfirmed(float Damage, bool bHeadshot, bool bKilled);
	float GetLastHitConfirmTime() const { return LastHitConfirmTime; }
	float GetLastHitDamage() const { return LastHitDamage; }
	bool WasLastHitHeadshot() const { return bLastHitHeadshot; }
	bool WasLastHitKill() const { return bLastHitKill; }

protected:
	virtual void BeginPlay() override;

	/** Server: called when this character dies. Base informs the game mode. */
	virtual void OnKilled(AController* Killer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NullAction|Health")
	TObjectPtr<UNAHealthComponent> HealthComponent;

	/** Max ground speed per gait, in cm/s. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float WalkSpeed = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float RunSpeed = 420.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float SprintSpeed = 680.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float CrouchSpeed = 170.f;

	/** Movement speed multiplier while aiming down sights. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float AimSpeedMultiplier = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float ProneSpeed = 70.f;

	/** Below this share of health the character limps (slower). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float InjuredHealthFraction = 0.35f;

	/** Falls faster than this (cm/s) on landing hurt. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float SafeLandingSpeed = 1100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Prone, BlueprintReadOnly, Category = "NullAction|Movement")
	bool bIsProne = false;

	UFUNCTION()
	void OnRep_Prone();

	UFUNCTION(Server, Reliable)
	void ServerSetProne(bool bNewProne);

	UPROPERTY(ReplicatedUsing = OnRep_Gait, BlueprintReadOnly, Category = "NullAction|Movement")
	ENAGait Gait = ENAGait::Run;

	UFUNCTION()
	void OnRep_Gait();

	UFUNCTION(Server, Reliable)
	void ServerSetGait(ENAGait NewGait);

	void ApplyGait();

	/** Weapons given to this character when it spawns (server). */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Weapons")
	TArray<TSubclassOf<ANAWeapon>> DefaultWeapons;

	UPROPERTY(ReplicatedUsing = OnRep_Weapons)
	TArray<TObjectPtr<ANAWeapon>> Weapons;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon)
	TObjectPtr<ANAWeapon> CurrentWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bIsAiming = false;

	UFUNCTION()
	void OnRep_Weapons();

	UFUNCTION()
	void OnRep_CurrentWeapon();

	UFUNCTION()
	void OnRep_Aiming();

	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(ANAWeapon* Weapon);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);

	/** Body mesh and animation Blueprint, loaded only if the asset exists (see Docs/ARCHITECTURE.md). */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Appearance")
	TSoftObjectPtr<USkeletalMesh> BodyMesh;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Appearance")
	TSoftClassPtr<UAnimInstance> BodyAnimClass;

	/** Simple stand-in body, shown only when no skeletal mesh is available, so the game is playable either way. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NullAction|Appearance")
	TObjectPtr<UStaticMeshComponent> FallbackBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NullAction|Appearance")
	TObjectPtr<UStaticMeshComponent> FallbackVisor;

	bool HasRealBody() const;

private:
	UFUNCTION()
	void HandleDeathStateChanged(bool bNowDead);

	UFUNCTION()
	void HandleKilled(AActor* Victim, AController* Killer);

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float Delta);

	void ResolveAppearance();
	void UpdateFallbackBodyHeight();
	void UpdateRotationMode();
	void AttachWeapon(ANAWeapon* Weapon);

	FVector DefaultMeshRelativeLocation = FVector::ZeroVector;
	FRotator DefaultMeshRelativeRotation = FRotator::ZeroRotator;
	FName DefaultMeshCollisionProfile = NAME_None;
	TEnumAsByte<ECollisionResponse> DefaultCapsuleVisibilityResponse = ECR_Block;

	float InvulnerableUntil = -100.f;
	float LastHitConfirmTime = -100.f;
	float LastHitDamage = 0.f;
	bool bLastHitHeadshot = false;
	bool bLastHitKill = false;
};
