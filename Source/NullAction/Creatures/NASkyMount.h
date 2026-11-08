#pragma once

#include "CoreMinimal.h"
#include "Creatures/NAHorse.h"
#include "NASkyMount.generated.h"

/**
 * ASH GLIDER: a large original flying creature that roosts on the Grey Peaks foothills.
 *  - Wild: perches at its roost, then takes off and circles high above it, then lands again. Running near it
 *    makes it take off. Tame it while it's perched by feeding Raw or Cooked Meat (3 feeds).
 *  - Riding (owner only): on the ground Space takes off. In the air: W flies where you look, A/D strafe,
 *    Space climbs, C dives, Shift boosts (stamina), no input glides and slowly sinks. Get within 4 m of the
 *    ground and it lands by itself; E dismounts on the ground. Stamina at 0: it can only glide down.
 *  - Storms (thunder, blizzard, dust, wind) buffet it and slow it: flying in bad weather is dangerous.
 * Reuses the horse's taming, ownership, rider and stamina systems; only the body and flight are new.
 */
UCLASS()
class NULLACTION_API ANASkyMount : public ANAHorse
{
	GENERATED_BODY()

public:
	ANASkyMount(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	bool IsFlying() const;
	float GetAltitudeAboveGround() const;
	bool IsInStorm() const;

protected:
	virtual void BeginPlay() override;
	virtual FString GetMountName() const override { return TEXT("Ash Glider"); }
	virtual const FNAModelSlot& GetModelSlot() const override;
	virtual USceneComponent* GetShapeRoot() const override { return GliderRoot; }
	virtual bool HasFood(const UNAInventoryComponent* Inventory) const override;
	virtual bool ConsumeFood(UNAInventoryComponent* Inventory) const override;
	virtual FString FoodHint() const override { return TEXT("bring Raw or Cooked Meat while it's perched"); }
	virtual bool CanInteract(const ANACharacterBase* User) const override;
	virtual float GetFleeSpeed() const override { return 1400.f; }

	virtual void ThinkServer(float DeltaSeconds) override;
	virtual void UpdateSpeed() override;
	virtual void AnimateBody(float DeltaSeconds) override;
	virtual void Input_Move(const FInputActionValue& Value) override;
	virtual void Input_Jump() override;
	virtual void Input_JumpReleased() override;
	virtual void Input_DescendStarted() override;
	virtual void Input_DescendCompleted() override;
	virtual void Input_Dismount() override;

	UFUNCTION(Server, Reliable) void ServerTakeOff();
	UFUNCTION(Server, Reliable) void ServerLand();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> GliderRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> WingLeft;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> WingRight;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> GliderAsh;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> GliderEmber;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> GliderTack;

private:
	void TakeOff();
	void Land();
	void FlyToward(const FVector& Destination, float Scale);

	bool bClimbHeld = false;
	bool bDiveHeld = false;
	float PhaseEndTime = 0.f;
	float CircleAngle = 0.f;
	float WingPhase = 0.f;
	bool bWildAirborne = false;
};
