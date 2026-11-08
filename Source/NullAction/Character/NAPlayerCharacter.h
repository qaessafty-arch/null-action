#pragma once

#include "CoreMinimal.h"
#include "Character/NACharacterBase.h"
#include "Building/NABuildPiece.h"
#include "Interaction/NAInteractable.h"
#include "NAPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UNASurvivalComponent;
class UNAInventoryComponent;
class UNAAbilityComponent;
class ANAHorse;
class UStaticMeshComponent;

struct FInputActionValue;

/**
 * The player-controlled human. Adds an over-the-shoulder camera, Enhanced Input, survival needs
 * and the interaction look-trace on top of the shared character base.
 *
 * Input assets can be authored in the editor and assigned in a Blueprint subclass. If MappingContext
 * is left empty, a complete default keyboard/mouse + gamepad scheme is built in code.
 */
UCLASS()
class NULLACTION_API ANAPlayerCharacter : public ANACharacterBase, public INAInteractable
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
	virtual bool IsInWater() const override;
	virtual bool IsHeadUnderwater() const override;

	/** Local: is the camera under the water surface (for the HUD tint)? */
	bool IsCameraUnderwater() const;
	bool IsSwimming() const;
	bool IsSliding() const { return bSliding; }

	/** Server only (owner/admin command): free flight. */
	void SetAdminFlying(bool bNewFlying);
	bool IsAdminFlying() const { return bAdminFlying; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	UNASurvivalComponent* GetSurvivalComponent() const { return SurvivalComponent; }
	UNAInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
	UNAAbilityComponent* GetAbilityComponent() const { return AbilityComponent; }

	// ----- Rescue (NAPlayerCharacter_Rescue.cpp) -----
	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual bool CanInteract(const ANACharacterBase* User) const override;
	virtual void Interact(ANACharacterBase* User) override;
	virtual void OnTookDamageHook() override;
	virtual void OnFiredWeaponHook() override;
	float GetReviveProgress() const { return ReviveProgress; }
	ANAPlayerCharacter* GetCarriedPlayer() const { return CarriedPlayer; }
	bool IsBeingCarried() const { return CarriedBy != nullptr; }
	bool IsRecovering() const;
	bool IsCloaked() const;

	// ----- Horse (Creatures/NAHorse) -----
	void SetMountedHorse(ANAHorse* Horse);
	ANAHorse* GetMountedHorse() const;

	// ----- Inventory menu (local UI state, read by the HUD) -----
	bool IsInventoryOpen() const { return bInventoryOpen; }
	int32 GetInventoryTab() const { return InventoryTab; }       // 0 = items, 1 = crafting
	int32 GetInventoryIndex() const { return InventoryIndex; }

	// ----- Gathering / building (local) -----
	/** Non-empty when looking at a tree or boulder you can harvest. */
	FString GetGatherPrompt() const { return GatherPrompt; }
	bool IsBuilding() const { return bBuildMode; }
	FName GetBuildItem() const { return BuildItem; }
	bool IsPlacementValid() const { return bPlacementValid; }

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NullAction|Inventory")
	TObjectPtr<UNAInventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BuildPreview;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NullAction|Ability")
	TObjectPtr<UNAAbilityComponent> AbilityComponent;

	/** Revive progress 0..1 from teammates' steady E taps (CPR rhythm). */
	UPROPERTY(Replicated) float ReviveProgress = 0.f;
	/** Server time until the post-revive weakness wears off. */
	UPROPERTY(Replicated) float RecoveringUntil = -1.f;
	UPROPERTY(ReplicatedUsing = OnRep_CarriedBy) TObjectPtr<ANAPlayerCharacter> CarriedBy;
	UPROPERTY(Replicated) TObjectPtr<ANAPlayerCharacter> CarriedPlayer;

	UFUNCTION() void OnRep_CarriedBy();
	UFUNCTION(Server, Reliable) void ServerDropCarried();

	virtual float GetExtraSpeedMultiplier() const override;
	virtual void OnDownedChanged(bool bNowDowned) override;

	UPROPERTY(EditDefaultsOnly, Category = "Camera") float DefaultFieldOfView = 85.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera") float StandingArmLength = 340.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera") float SprintArmLength = 390.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera") float CameraInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Survival") float JumpStaminaCost = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Movement") float MantleStaminaCost = 10.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Movement") float MaxMantleHeight = 260.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Movement") float DodgeStaminaCost = 20.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Movement") float DodgeSpeed = 950.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Movement") float DodgeCooldownSeconds = 0.9f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Movement") float SlideBoost = 250.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Movement") float SlideSeconds = 0.9f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Movement") float SwimSpeed = 300.f;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Movement") float FastSwimSpeed = 450.f;

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
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> ProneAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> DodgeAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> ScoreboardAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> InventoryAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> MenuUpAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> MenuDownAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> MenuLeftAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> MenuRightAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> MenuConfirmAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> AbilityAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> HelpAction;
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Input") TObjectPtr<UInputAction> WhistleAction;

	UPROPERTY(ReplicatedUsing = OnRep_AdminFlying)
	bool bAdminFlying = false;

	UFUNCTION()
	void OnRep_AdminFlying();

	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* Target);

	UFUNCTION(Server, Reliable)
	void ServerMantle();

	UFUNCTION(Server, Reliable)
	void ServerSlide();

	UFUNCTION(Server, Reliable)
	void ServerDodge(FVector_NetQuantizeNormal Direction);

	UFUNCTION(Server, Reliable)
	void ServerGather(FVector_NetQuantize Location);

	UFUNCTION(Server, Reliable)
	void ServerPlace(FName ItemId, FVector_NetQuantize AimPoint, float Yaw);

private:
	void BuildDefaultInputIfNeeded();
	void UpdateDesiredGait();
	void UpdateCamera(float DeltaSeconds);
	void UpdateInteractionFocus();
	void UpdateFootstepNoise();
	void UpdateSwimming();
	bool FindMantle(FVector& OutLaunchVelocity) const;
	void ExecuteMantle(const FVector& LaunchVelocity);
	bool CanSlide() const;
	void StartSlide();
	void EndSlide();
	bool CanDodge() const;
	void ExecuteDodge(const FVector& Direction);

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
	void Input_WeaponSlot5();
	void Input_CrouchCompleted();
	void Input_Prone();
	void Input_Dodge();
	void Input_ScoreboardStarted();
	void Input_ScoreboardCompleted();
	void Input_Inventory();
	void Input_MenuUp();
	void Input_MenuDown();
	void Input_MenuLeft();
	void Input_MenuRight();
	void Input_MenuConfirm();
	void Input_Ability();
	void Input_Help();
	void Input_Whistle();
	UFUNCTION(Server, Reliable) void ServerWhistle();
	TWeakObjectPtr<ANAHorse> MountedHorse; // Server.

	// NAPlayerCharacter_Rescue.cpp
	void UpdateRescue(float DeltaSeconds);
	void StartCarrying(ANAPlayerCharacter* Downed);
	void DropCarried();
	void ApplyCarriedState();
	void CompleteRevive();
	float ServerTimeNow() const;

	// NAPlayerCharacter_Survival.cpp
	void UpdateGatherFocus(const FHitResult& Hit);
	void UpdateBuildPreview();
	bool ComputePlacement(ENABuildPieceType Type, const FVector& AimPoint, float Yaw, FTransform& OutTransform) const;
	void StartBuilding(FName ItemId);
	void StopBuilding();
	int32 GetMenuEntryCount() const;

	TWeakObjectPtr<AActor> FocusedInteractable;
	float NextFootstepNoiseTime = 0.f;
	bool bSwimAscend = false;
	bool bSwimDescend = false;
	bool bSliding = false;
	float LastDodgeTime = -100.f;
	float LastGatherTime = -100.f;
	float LastReviveTapTime = -100.f;
	float NextSpeedRefresh = 0.f;
	float LastSpeedMultiplier = 1.f;
	bool bWasCloaked = false;
	bool bInventoryOpen = false;
	int32 InventoryTab = 0;
	int32 InventoryIndex = 0;
	FString GatherPrompt;
	FVector GatherLocation = FVector::ZeroVector;
	bool bBuildMode = false;
	FName BuildItem;
	float BuildYaw = 0.f;
	bool bPlacementValid = false;
	FVector BuildAimPoint = FVector::ZeroVector;
	float DefaultGroundFriction = 8.f;
	float DefaultBrakingDeceleration = 1800.f;
	FTimerHandle SlideTimer;
	bool bSprintHeld = false;
	bool bWalkToggled = false;
};
