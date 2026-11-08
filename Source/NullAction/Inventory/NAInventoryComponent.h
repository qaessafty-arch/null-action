#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NAInventoryComponent.generated.h"

struct FNARecipe;

USTRUCT()
struct FNAItemStack
{
	GENERATED_BODY()

	UPROPERTY() FName ItemId;
	UPROPERTY() int32 Count = 0;
};

/**
 * Resource and item inventory. The server owns it; only the owning player gets it replicated.
 * Clients ask to craft or use; the server checks the ingredients and applies the result, so items
 * can't be duplicated or invented by a modified client.
 */
UCLASS(ClassGroup = (NullAction), meta = (BlueprintSpawnableComponent))
class NULLACTION_API UNAInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNAInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	const TArray<FNAItemStack>& GetItems() const { return Items; }
	int32 CountOf(FName ItemId) const;
	bool CanCraft(const FNARecipe& Recipe) const;
	bool IsNearCampfire() const;

	/** Server only. Returns how many were actually added/removed. */
	int32 AddItem(FName ItemId, int32 Count, bool bNotify = true);
	bool RemoveItem(FName ItemId, int32 Count);

	/** Any machine: the owning client asks the server. */
	void RequestCraft(FName RecipeId);
	void RequestUse(FName ItemId);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Inventory")
	int32 MaxSlots = 24;

	UPROPERTY(Replicated)
	TArray<FNAItemStack> Items;

	UFUNCTION(Server, Reliable)
	void ServerCraft(FName RecipeId);

	UFUNCTION(Server, Reliable)
	void ServerUse(FName ItemId);

private:
	void Craft(FName RecipeId);
	void Use(FName ItemId);
	void Tell(const FString& Message) const;
};
