#pragma once

#include "CoreMinimal.h"
#include "Character/NACharacterBase.h"
#include "NAPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UNASurvivalComponent;
struct FInputActionValue;

/**
 * The player-controlled human. Adds an over-the-shoulder camera, Enhanced Input, survival needs
 * and the interaction look-trace on top of the shared character base.
 *
 * Input assets can be authored in the editor and assigned in a Blueprint subclass. If MappingContext
 * is left empty, a complete default keyboard/mouse + gamepad scheme is built in code.
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
	virtual bool CanSprint() const override;
	virtual void GetAimViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
	virtual void ApplyRecoil(float PitchDegrees, float YawDegrees) override;
	virtual void OnJumped_Implementation() override;

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	UNASurvivalComponent* GetSurvivalComponent() const { return SurvivalComponent; }

	/** Interactable the player is currently looking at (local only), or null. */
	AActor* GetFocusedInteractable() const { return FocusedInteractable.Get(); }

	UFUNCTION(BlueprintPure, Category = "NullAction|Movement")
	bool IsWalkToggled() const { return bWalkToggled; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NullAction|Survival")
	TObjectPtr<UNASurvivalComponent> SurvivalComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Camera") float DefaultFieldOfView = 85.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera") float StandingArmLength = 340.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera") float SprintArmLength = 390.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera") float CameraInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival") float JumpStaminaCost = 8.f;

	/** How far from the character the interact key reaches, in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Interaction") float InteractionRange = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> WalkAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> FireAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> AimAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> InteractAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> NextWeaponAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> PreviousWeaponAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TArray<TObjectPtr<UInputAction>> WeaponSlotActions;

	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* Target);

private:
	void BuildDefaultInputIfNeeded();
	void UpdateDesiredGait();
	void UpdateCamera(float DeltaSeconds);
	void UpdateInteractionFocus();

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_JumpStarted();
	void Input_JumpCompleted();
	void Input_SprintStarted();
	void Input_SprintCompleted();
	void Input_WalkToggle();
	void Input_Crouch();
	void Input_FireStarted();
	void Input_FireCompleted();
	void Input_AimStarted();
	void Input_AimCompleted();
	void Input_Reload();
	void Input_Interact();
	void Input_NextWeapon();
	void Input_PreviousWeapon();
	void Input_WeaponSlot1();
	void Input_WeaponSlot2();
	void Input_WeaponSlot3();
	void Input_WeaponSlot4();

	TWeakObjectPtr<AActor> FocusedInteractable;
	bool bSprintHeld = false;
	bool bWalkToggled = false;
};
