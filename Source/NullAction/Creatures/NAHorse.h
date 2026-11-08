#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/NAInteractable.h"
#include "NAHorse.generated.h"

class UNAHealthComponent;
class UStaticMeshComponent;
class UStaticMesh;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class ANAPlayerCharacter;
struct FInputActionValue;

/**
 * Horse: wild herds roam the Ashen Grassland.
 *  - Wild: grazes and wanders near its herd. Running near it (faster than a walk) spooks it and it bolts.
 *    Walk (Alt) or crouch up to it and press E with Berries or Ashroot Herbs to feed it; three feeds tame it.
 *  - Tamed: belongs to that player (by stable account id). E to ride; H whistles it over from up to 150 m.
 *  - Riding: the player's controller possesses the horse. WASD, Shift gallops (uses horse stamina),
 *    Space jumps, E dismounts. Slopes and water slow it. If the rider goes down or the horse dies,
 *    the rider is thrown off.
 * Server-authoritative; trust, ownership, stamina, gallop and rider replicate.
 */
UCLASS()
class NULLACTION_API ANAHorse : public ACharacter, public INAInteractable
{
	GENERATED_BODY()

public:
	ANAHorse(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual bool CanInteract(const ANACharacterBase* User) const override;
	virtual void Interact(ANACharacterBase* User) override;

	bool IsDead() const;
	bool IsTamed() const { return bTamed; }
	ANAPlayerCharacter* GetRider() const { return Rider; }
	const FString& GetOwnerName() const { return OwnerName; }
	float GetStaminaFraction() const { return Stamina / MaxStamina; }
	float GetTrust() const { return Trust; }
	bool IsGalloping() const { return bGalloping; }
	UNAHealthComponent* GetHealthComponent() const { return HealthComponent; }

	/** Server. */
	void SetHerdCenter(const FVector& Center) { HerdCenter = Center; }
	void SetCoat(uint8 NewCoat) { CoatIndex = NewCoat; }
	void CallTo(APawn* Caller);
	void Dismount();
	static ANAHorse* FindOwnedHorse(UWorld* World, const FString& OwnerId);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UNAHealthComponent> HealthComponent;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> VisualRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> NeckJoint;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> TailJoint;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SaddlePoint;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Hips;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Knees;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CoatParts;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ManeParts;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> HoofParts;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> TackParts; // Saddle and bridle: shown once tamed.

	UPROPERTY(ReplicatedUsing = OnRep_Tamed) bool bTamed = false;
	UPROPERTY(Replicated) FString OwnerName;
	UPROPERTY(Replicated) float Trust = 0.f;
	UPROPERTY(Replicated) float Stamina = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Gallop) bool bGalloping = false;
	UPROPERTY(ReplicatedUsing = OnRep_Coat) uint8 CoatIndex = 0;
	UPROPERTY(ReplicatedUsing = OnRep_Rider) TObjectPtr<ANAPlayerCharacter> Rider;

	UFUNCTION() void OnRep_Tamed();
	UFUNCTION() void OnRep_Gallop();
	UFUNCTION() void OnRep_Coat();
	UFUNCTION() void OnRep_Rider();
	UFUNCTION() void HandleKilled(AActor* Victim, AController* Killer);

	UFUNCTION(Server, Reliable) void ServerSetGallop(bool bNewGallop);
	UFUNCTION(Server, Reliable) void ServerDismount();

	// Input (built in code, like the player's).
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> RideContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> GallopAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> DismountAction;

	static constexpr float MaxStamina = 100.f;

private:
	UStaticMeshComponent* MakePart(FName PartName, USceneComponent* Parent, UStaticMesh* Shape, const FVector& Location,
		const FVector& SizeCm, const FRotator& Rotation, TArray<TObjectPtr<UStaticMeshComponent>>& Group);
	void BuildInput();
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_GallopStarted();
	void Input_GallopCompleted();
	void Input_Jump();
	void Input_Dismount();

	void Mount(ANAPlayerCharacter* NewRider);
	void ApplyRiderAttachment(ANAPlayerCharacter* Seated, bool bSeat);
	void ThinkServer(float DeltaSeconds);
	void UpdateSpeed();
	float TerrainSpeedFactor() const;
	void AnimateBody(float DeltaSeconds);
	void MoveToward(const FVector& Destination, float Scale);

	FString OwnerId;
	FVector HerdCenter = FVector::ZeroVector;
	FVector WanderTarget = FVector::ZeroVector;
	float NextWanderTime = 0.f;
	float FleeUntil = 0.f;
	FVector FleeDirection = FVector::ForwardVector;
	float EatUntil = 0.f;
	TWeakObjectPtr<APawn> CallTarget;
	float CallUntil = 0.f;
	float GaitPhase = 0.f;
	/** Clients: who this machine currently shows in the saddle (to undo it when Rider changes). */
	TWeakObjectPtr<ANAPlayerCharacter> SeatedLocally;
};
