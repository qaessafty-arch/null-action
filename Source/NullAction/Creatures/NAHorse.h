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
class UNAInventoryComponent;
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

	// Species hooks (ANADeer overrides these).
	virtual bool CanBeTamed() const { return true; }
	virtual bool ShouldSpookFrom(const ANAPlayerCharacter* Player, float Distance) const;
	virtual float GetFleeSpeed() const;
	virtual void DropLoot();
	virtual FString GetMountName() const { return TEXT("horse"); }
	/** Server: take one food item for taming; returns false if the player has none. */
	virtual bool ConsumeFood(class UNAInventoryComponent* Inventory) const;
	virtual bool HasFood(const class UNAInventoryComponent* Inventory) const;
	virtual FString FoodHint() const { return TEXT("bring Berries or Ashroot Herbs"); }

	// Movement / input hooks (ANASkyMount flies).
	virtual void ThinkServer(float DeltaSeconds);
	virtual void UpdateSpeed();
	virtual void AnimateBody(float DeltaSeconds);
	virtual void Input_Move(const FInputActionValue& Value);
	virtual void Input_Jump();
	virtual void Input_JumpReleased() {}
	virtual void Input_DescendStarted() {}
	virtual void Input_DescendCompleted() {}
	virtual void Input_Dismount();
	void MoveToward(const FVector& Destination, float Scale);
	float TerrainSpeedFactor() const;

	UPROPERTY(Transient) TObjectPtr<UInputAction> DescendAction;

	// Wild/tamed state shared with subclasses.
	FVector HerdCenter = FVector::ZeroVector;
	TWeakObjectPtr<APawn> CallTarget;
	float CallUntil = 0.f;
	float FleeUntil = 0.f;
	FVector FleeDirection = FVector::ForwardVector;
	float EatUntil = 0.f;

	UStaticMeshComponent* MakePart(FName PartName, USceneComponent* Parent, UStaticMesh* Shape, const FVector& Location,
		const FVector& SizeCm, const FRotator& Rotation, TArray<TObjectPtr<UStaticMeshComponent>>& Group);

private:
	void BuildInput();
	void Input_Look(const FInputActionValue& Value);
	void Input_GallopStarted();
	void Input_GallopCompleted();

	void Mount(ANAPlayerCharacter* NewRider);
	void ApplyRiderAttachment(ANAPlayerCharacter* Seated, bool bSeat);

	FString OwnerId;
	FVector WanderTarget = FVector::ZeroVector;
	float NextWanderTime = 0.f;
	float GaitPhase = 0.f;
	/** Clients: who this machine currently shows in the saddle (to undo it when Rider changes). */
	TWeakObjectPtr<ANAPlayerCharacter> SeatedLocally;
};
