#pragma once

#include "CoreMinimal.h"

/** What an item is for. */
enum class ENAItemCategory : uint8
{
	Resource,
	Consumable,
	Placeable
};

struct FNAItemDef
{
	FName Id;
	FString DisplayName;
	ENAItemCategory Category = ENAItemCategory::Resource;
	int32 MaxStack = 50;
	FLinearColor Color = FLinearColor::White;
	FString Description;
};

struct FNAIngredient
{
	FName Item;
	int32 Count = 1;
};

struct FNARecipe
{
	FName Id;
	TArray<FNAIngredient> Inputs;
	FName Output;
	int32 OutputCount = 1;
	bool bNeedsCampfire = false;
};

/**
 * Item and recipe registry. Adding an item or recipe = adding one entry in NAItems.cpp; the inventory,
 * crafting, HUD and building code all read from here. (Moves to a DataTable once designers edit it in-editor.)
 */
namespace NAItems
{
	NULLACTION_API const TArray<FNAItemDef>& All();
	NULLACTION_API const FNAItemDef* Find(FName Id);
	NULLACTION_API const TArray<FNARecipe>& Recipes();
	NULLACTION_API const FNARecipe* FindRecipe(FName Id);
	NULLACTION_API FString NameOf(FName Id);

	// Well-known ids used in code.
	inline FName Wood()       { return TEXT("Wood"); }
	inline FName Stone()      { return TEXT("Stone"); }
	inline FName Herb()       { return TEXT("Herb"); }
	inline FName Berries()    { return TEXT("Berries"); }
	inline FName Scrap()      { return TEXT("ScrapMetal"); }
	inline FName RawMeat()    { return TEXT("RawMeat"); }
	inline FName AshHide()    { return TEXT("AshHide"); }
	inline FName EmberFang()  { return TEXT("EmberFang"); }
	inline FName Foundation() { return TEXT("WoodFoundation"); }
	inline FName Wall()       { return TEXT("WoodWall"); }
	inline FName Doorway()    { return TEXT("WoodDoorway"); }
	inline FName Campfire()   { return TEXT("Campfire"); }
	inline FName HuntingKnife() { return TEXT("HuntingKnife"); }
}
