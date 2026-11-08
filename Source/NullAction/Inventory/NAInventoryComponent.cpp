#include "Inventory/NAInventoryComponent.h"
#include "Hunting/NAHuntTrap.h"
#include "Hunting/NABait.h"
#include "Weapons/NAWeaponTypes.h"

#include "Building/NABuildPiece.h"
#include "Character/NACharacterBase.h"
#include "Combat/NAHealthComponent.h"
#include "Core/NAPlayerController.h"
#include "Core/NAPlayerState.h"
#include "Core/NAGameMode.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Inventory/NAItems.h"
#include "Net/UnrealNetwork.h"
#include "Survival/NASurvivalComponent.h"
#include "Weapons/NAWeapon.h"

UNAInventoryComponent::UNAInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNAInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UNAInventoryComponent, Items, COND_OwnerOnly);
}

int32 UNAInventoryComponent::CountOf(FName ItemId) const
{
	int32 Total = 0;
	for (const FNAItemStack& Stack : Items)
	{
		if (Stack.ItemId == ItemId)
		{
			Total += Stack.Count;
		}
	}
	return Total;
}

bool UNAInventoryComponent::IsNearCampfire() const
{
	const AActor* Owner = GetOwner();
	return Owner && ANABuildPiece::FindCampfireNear(Owner->GetWorld(), Owner->GetActorLocation(), 450.f) != nullptr;
}

bool UNAInventoryComponent::CanCraft(const FNARecipe& Recipe) const
{
	if (Recipe.bNeedsCampfire && !IsNearCampfire())
	{
		return false;
	}
	for (const FNAIngredient& Input : Recipe.Inputs)
	{
		if (CountOf(Input.Item) < Input.Count)
		{
			return false;
		}
	}
	return true;
}

int32 UNAInventoryComponent::AddItem(FName ItemId, int32 Count, bool bNotify)
{
	const FNAItemDef* Def = NAItems::Find(ItemId);
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Def || Count <= 0)
	{
		return 0;
	}

	int32 Remaining = Count;
	// Top up existing stacks first, then open new slots.
	for (FNAItemStack& Stack : Items)
	{
		if (Stack.ItemId == ItemId && Stack.Count < Def->MaxStack)
		{
			const int32 Moved = FMath::Min(Remaining, Def->MaxStack - Stack.Count);
			Stack.Count += Moved;
			Remaining -= Moved;
			if (Remaining == 0) { break; }
		}
	}
	while (Remaining > 0 && Items.Num() < MaxSlots)
	{
		FNAItemStack NewStack;
		NewStack.ItemId = ItemId;
		NewStack.Count = FMath::Min(Remaining, Def->MaxStack);
		Remaining -= NewStack.Count;
		Items.Add(NewStack);
	}

	const int32 Added = Count - Remaining;
	if (bNotify && Added > 0)
	{
		Tell(FString::Printf(TEXT("+%d %s"), Added, *Def->DisplayName));
	}
	if (Remaining > 0)
	{
		Tell(TEXT("Inventory full."));
	}
	return Added;
}

bool UNAInventoryComponent::RemoveItem(FName ItemId, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || CountOf(ItemId) < Count)
	{
		return false;
	}
	int32 Remaining = Count;
	for (int32 Index = Items.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		if (Items[Index].ItemId != ItemId)
		{
			continue;
		}
		const int32 Taken = FMath::Min(Remaining, Items[Index].Count);
		Items[Index].Count -= Taken;
		Remaining -= Taken;
		if (Items[Index].Count <= 0)
		{
			Items.RemoveAt(Index);
		}
	}
	return true;
}

void UNAInventoryComponent::RequestCraft(FName RecipeId)
{
	if (GetOwner() && GetOwner()->HasAuthority()) { Craft(RecipeId); }
	else { ServerCraft(RecipeId); }
}

void UNAInventoryComponent::RequestUse(FName ItemId)
{
	if (GetOwner() && GetOwner()->HasAuthority()) { Use(ItemId); }
	else { ServerUse(ItemId); }
}

void UNAInventoryComponent::ServerCraft_Implementation(FName RecipeId) { Craft(RecipeId); }
void UNAInventoryComponent::ServerUse_Implementation(FName ItemId)     { Use(ItemId); }

void UNAInventoryComponent::Craft(FName RecipeId)
{
	const FNARecipe* Recipe = NAItems::FindRecipe(RecipeId);
	const ANACharacterBase* Character = Cast<ANACharacterBase>(GetOwner());
	if (!Recipe || !Character || Character->IsDead())
	{
		return;
	}
	if (!CanCraft(*Recipe))
	{
		Tell(Recipe->bNeedsCampfire && !IsNearCampfire() ? TEXT("You need to be near a campfire.") : TEXT("Missing ingredients."));
		return;
	}
	for (const FNAIngredient& Input : Recipe->Inputs)
	{
		RemoveItem(Input.Item, Input.Count);
	}
	AddItem(Recipe->Output, Recipe->OutputCount, false);
	Tell(FString::Printf(TEXT("Crafted %s."), *NAItems::NameOf(Recipe->Output)));
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->AwardXP(Cast<APawn>(GetOwner()) ? Cast<APawn>(GetOwner())->GetController() : nullptr, 5, TEXT("Crafting"));
	}
}

void UNAInventoryComponent::Use(FName ItemId)
{
	ANACharacterBase* Character = Cast<ANACharacterBase>(GetOwner());
	const FNAItemDef* Def = NAItems::Find(ItemId);
	if (!Character || Character->IsDead() || Character->IsDowned() || !Def || Def->Category != ENAItemCategory::Consumable || CountOf(ItemId) <= 0)
	{
		return; // Placeables are used through build mode (server checks placement there).
	}

	UNAHealthComponent* Health = Character->GetHealthComponent();
	UNASurvivalComponent* Survival = Character->FindComponentByClass<UNASurvivalComponent>();
	bool bUsed = false;

	if (ItemId == TEXT("Bandage"))
	{
		const bool bStopped = Survival && Survival->StopBleeding();
		bUsed = (Health && Health->Heal(30.f) > 0.f) || bStopped;
	}
	else if (ItemId == TEXT("CookedMeat"))  { bUsed = Survival && Survival->RestoreFood(45.f) > 0.f; }
	else if (ItemId == NAItems::RawMeat())
	{
		bUsed = Survival && Survival->RestoreFood(12.f) > 0.f;
		if (bUsed && FMath::FRand() < 0.3f)
		{
			Survival->Poison(40.f); // Raw meat is a gamble; cook it.
			Tell(TEXT("Your stomach turns. You are POISONED (craft an Antidote: 2 Herbs + 1 Berries)."));
		}
	}
	else if (ItemId == TEXT("Antidote"))    { bUsed = Survival && Survival->CurePoison(); }
	else if (ItemId == TEXT("Medicine"))
	{
		const bool bCured = Survival && Survival->CureInfection();
		bUsed = (Health && Health->Heal(20.f) > 0.f) || bCured;
	}
	else if (ItemId == TEXT("RepairKit"))
	{
		ANAWeapon* Held = Character->GetCurrentWeapon();
		bUsed = Held && Held->GetDurability() < 99.f;
		if (bUsed) { Held->Repair(); }
	}
	else if (ItemId == TEXT("Rebreather"))  { bUsed = Survival && !Survival->HasRebreather(); if (bUsed) { Survival->SetRebreather(true); } }
	else if (ItemId == TEXT("FurCoat"))     { bUsed = Survival && !Survival->HasFurCoat(); if (bUsed) { Survival->SetFurCoat(true); } }
	else if (ItemId == NAItems::Berries())  { bUsed = Survival && (Survival->RestoreFood(8.f) + Survival->RestoreWater(15.f)) > 0.f; }
	else if (ItemId == TEXT("ArmorPlate"))  { bUsed = Health && Health->AddArmor(50.f) > 0.f; }
	else if (ItemId == TEXT("AmmoPack"))
	{
		for (ANAWeapon* Weapon : Character->GetWeapons())
		{
			if (Weapon && !Weapon->GetStats().bUsesArrows && Weapon->AddReserveAmmo(Weapon->GetStats().MagazineSize * 2) > 0) { bUsed = true; }
		}
	}
	else if (ItemId == TEXT("HuntingBow"))
	{
		bUsed = Character->GiveWeapon(ANAWeapon_HuntingBow::StaticClass(), true) != nullptr;
	}
	else if (ItemId == TEXT("ArrowBundle"))
	{
		for (ANAWeapon* Weapon : Character->GetWeapons())
		{
			if (Weapon && Weapon->GetStats().bUsesArrows && Weapon->AddReserveAmmo(6) > 0) { bUsed = true; }
		}
	}
	else if (ItemId == TEXT("SnareTrap"))
	{
		bUsed = ANAHuntTrap::Place(Character) != nullptr;
		if (!bUsed) { Tell(TEXT("Can't set a trap here (needs open ground, max 5 traps each).")); return; }
	}
	else if (ItemId == TEXT("Bait"))
	{
		bUsed = ANABait::Place(Character) != nullptr;
		if (!bUsed) { Tell(TEXT("Can't drop bait here.")); return; }
	}
	else if (ItemId == TEXT("AshenAegis"))
	{
		bUsed = Health != nullptr;
		if (Health)
		{
			Health->Heal(10000.f);
			Health->AddArmor(10000.f);
		}
		const APawn* Pawn = Cast<APawn>(Character);
		if (ANAPlayerState* State = Pawn ? Pawn->GetPlayerState<ANAPlayerState>() : nullptr)
		{
			State->SetAshCharges(State->GetAshCharges() + 1);
		}
	}
	else if (ItemId == TEXT("EmberTonic"))
	{
		bUsed = Health && Health->Heal(1000.f) >= 0.f;
		if (Survival) { Survival->WarmToNormal(); Survival->Rest(100.f); }
	}

	if (bUsed)
	{
		RemoveItem(ItemId, 1);
		Tell(FString::Printf(TEXT("Used %s."), *Def->DisplayName));
	}
	else
	{
		Tell(FString::Printf(TEXT("You don't need %s right now."), *Def->DisplayName));
	}
}

void UNAInventoryComponent::Tell(const FString& Message) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (ANAPlayerController* Player = Pawn ? Cast<ANAPlayerController>(Pawn->GetController()) : nullptr)
	{
		Player->ClientReceiveChat(ENAChatChannel::System, FString(), Message);
	}
}
