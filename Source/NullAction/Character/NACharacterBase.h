#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NACharacterBase.generated.h"

class UStaticMeshComponent;
class USkeletalMesh;
class UAnimInstance;

/** Locomotion gait. Crouching is handled separately by ACharacter's built-in, predicted crouch. */
UENUM(BlueprintType)
enum class ENAGait : uint8
{
	Walk,
	Run,
	Sprint
};

/**
 * Shared humanoid base for every NULL-ACTION character: players, enemy bots, NPCs.
 * Owns locomotion speeds, replicated gait, crouch and body appearance.
 * Player-only concerns (camera, input) live in ANAPlayerCharacter.
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

	/** Request a gait. Applied locally right away and sent to the server, which owns the replicated value. */
	UFUNCTION(BlueprintCallable, Category = "NullAction|Movement")
	void SetDesiredGait(ENAGait NewGait);

	UFUNCTION(BlueprintPure, Category = "NullAction|Movement")
	ENAGait GetGait() const { return Gait; }

	UFUNCTION(BlueprintPure, Category = "NullAction|Movement")
	bool IsSprinting() const { return Gait == ENAGait::Sprint && !bIsCrouched; }

	UFUNCTION(BlueprintCallable, Category = "NullAction|Movement")
	void ToggleCrouch();

protected:
	virtual void BeginPlay() override;

	/** Max ground speed per gait, in cm/s. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float WalkSpeed = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float RunSpeed = 420.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float SprintSpeed = 680.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NullAction|Movement")
	float CrouchSpeed = 170.f;

	UPROPERTY(ReplicatedUsing = OnRep_Gait, BlueprintReadOnly, Category = "NullAction|Movement")
	ENAGait Gait = ENAGait::Run;

	UFUNCTION()
	void OnRep_Gait();

	UFUNCTION(Server, Reliable)
	void ServerSetGait(ENAGait NewGait);

	void ApplyGait();

	/**
	 * Body mesh and animation Blueprint, loaded only if the asset exists.
	 * Defaults point at the Epic Mannequin from the free Third Person content pack
	 * (Content Browser > Add > Add Feature or Content Pack > Third Person).
	 * A Blueprint subclass can point these at any other licensed character.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Appearance")
	TSoftObjectPtr<USkeletalMesh> BodyMesh;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Appearance")
	TSoftClassPtr<UAnimInstance> BodyAnimClass;

	/** Simple stand-in body, shown only when no skeletal mesh is available, so the game is playable either way. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NullAction|Appearance")
	TObjectPtr<UStaticMeshComponent> FallbackBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NullAction|Appearance")
	TObjectPtr<UStaticMeshComponent> FallbackVisor;

private:
	void ResolveAppearance();
	void UpdateFallbackBodyHeight();
};
