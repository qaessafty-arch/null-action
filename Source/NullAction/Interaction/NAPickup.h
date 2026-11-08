#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NAInteractable.h"
#include "NAPickup.generated.h"

class ANAWeapon;
class USphereComponent;
class UStaticMeshComponent;

/** Base for world pickups: spinning item that is taken with the interact key and respawns later. */
UCLASS(Abstract)
class NULLACTION_API ANAPickupBase : public AActor, public INAInteractable
{
	GENERATED_BODY()

public:
	ANAPickupBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// INAInteractable
	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual bool CanInteract(const ANACharacterBase* User) const override { return bAvailable; }
	virtual void Interact(ANACharacterBase* User) override;

	/** Seconds until it reappears after being taken; 0 = never. Set before FinishSpawning. */
	void SetRespawnSeconds(float Seconds) { RespawnSeconds = Seconds; }

protected:
	virtual void BeginPlay() override;

	/** Server: give the item. Return false if the user can't take it (e.g. ammo already full). */
	virtual bool GiveTo(ANACharacterBase* User) { return false; }
	virtual FText GetItemName() const { return FText::GetEmpty(); }
	virtual FLinearColor GetItemColor() const { return FLinearColor::White; }
	virtual FVector GetItemSize() const { return FVector(25.f); }

	/** Seconds until it reappears after being taken; 0 = never. */
	UPROPERTY(EditAnywhere, Category = "NullAction|Pickup")
	float RespawnSeconds = 20.f;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Pickup")
	TObjectPtr<USphereComponent> InteractVolume;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Pickup")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Pickup")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Available)
	bool bAvailable = true;

	UFUNCTION()
	void OnRep_Available();

	void RefreshAppearance();

private:
	void Respawn();
	FTimerHandle RespawnTimer;
};

UCLASS()
class NULLACTION_API ANAWeaponPickup : public ANAPickupBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, Replicated, Category = "NullAction|Pickup")
	TSubclassOf<ANAWeapon> WeaponClass;

protected:
	virtual bool GiveTo(ANACharacterBase* User) override;
	virtual FText GetItemName() const override;
	virtual FLinearColor GetItemColor() const override { return FLinearColor(0.12f, 0.12f, 0.13f); }
	virtual FVector GetItemSize() const override;
};

UENUM(BlueprintType)
enum class ENASupplyType : uint8
{
	Ammo,
	Medkit,
	Armor,
	Food,
	Water
};

UCLASS()
class NULLACTION_API ANASupplyPickup : public ANAPickupBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, Replicated, Category = "NullAction|Pickup")
	ENASupplyType SupplyType = ENASupplyType::Ammo;

	/** Health/armor/food/water points, or magazines of ammo for every carried weapon. */
	UPROPERTY(EditAnywhere, Category = "NullAction|Pickup")
	float Amount = 50.f;

protected:
	virtual bool GiveTo(ANACharacterBase* User) override;
	virtual FText GetItemName() const override;
	virtual FLinearColor GetItemColor() const override;
};
