#pragma once

#include "CoreMinimal.h"
#include "Character/NACharacterBase.h"
#include "NAPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * The player-controlled human. Adds an over-the-shoulder third-person camera and Enhanced Input.
 *
 * Input assets can be authored in the editor and assigned in a Blueprint subclass. If MappingContext
 * is left empty, a complete default keyboard/mouse + gamepad scheme is built in code, so the
 * character is playable with zero editor assets.
 */
UCLASS()
class NULLACTION_API ANAPlayerCharacter : public ANACharacterBase
{
	GENERATED_BODY()

public:
	ANAPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void Tick(float DeltaSeconds) override;

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintPure, Category = "NullAction|Movement")
	bool IsWalkToggled() const { return bWalkToggled; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Camera boom length standing / crouched; the boom eases between them. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float StandingArmLength = 340.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float SprintArmLength = 390.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float ArmLengthInterpSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input")
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input")
	TObjectPtr<UInputAction> CrouchAction;

private:
	void BuildDefaultInputIfNeeded();
	void UpdateDesiredGait();

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_JumpStarted();
	void Input_JumpCompleted();
	void Input_SprintStarted();
	void Input_SprintCompleted();
	void Input_WalkToggle();
	void Input_Crouch();

	bool bSprintHeld = false;
	bool bWalkToggled = false;
};
