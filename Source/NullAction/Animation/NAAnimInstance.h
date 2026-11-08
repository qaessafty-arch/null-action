#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Character/NACharacterBase.h"
#include "NAAnimInstance.generated.h"

/**
 * C++ parent for NULL-ACTION character Animation Blueprints.
 * Computes locomotion values once per frame in native code so the AnimGraph only reads them.
 * Works the same on every machine because it reads replicated character state.
 */
UCLASS()
class NULLACTION_API UNAAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "NullAction|Locomotion")
	TObjectPtr<ANACharacterBase> OwningCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "NullAction|Locomotion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "NullAction|Locomotion")
	float GroundSpeed = 0.f;

	/** Movement direction relative to facing, -180..180 degrees. Feeds strafe blendspaces. */
	UPROPERTY(BlueprintReadOnly, Category = "NullAction|Locomotion")
	float Direction = 0.f;

	/** Aim pitch relative to the body, for aim offsets. Replicated by the engine for remote players. */
	UPROPERTY(BlueprintReadOnly, Category = "NullAction|Locomotion")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "NullAction|Locomotion")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "NullAction|Locomotion")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "NullAction|Locomotion")
	bool bIsCrouching = false;

	UPROPERTY(BlueprintReadOnly, Category = "NullAction|Locomotion")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "NullAction|Locomotion")
	ENAGait Gait = ENAGait::Run;
};
