#include "Inventory/NAItems.h"

namespace
{
	TArray<FNAItemDef> BuildItems()
	{
		using C = ENAItemCategory;
		auto Item = [](const TCHAR* Id, const TCHAR* Name, C Category, int32 MaxStack, const FLinearColor& Color, const TCHAR* Description)
		{
			FNAItemDef Def;
			Def.Id = Id;
			Def.DisplayName = Name;
			Def.Category = Category;
			Def.MaxStack = MaxStack;
			Def.Color = Color;
			Def.Description = Description;
			return Def;
		};
		return {
			// Resources
			Item(TEXT("Wood"),       TEXT("Wood"),        C::Resource, 100, FLinearColor(0.35f, 0.2f, 0.1f),   TEXT("Chopped from trees. Building and fires.")),
			Item(TEXT("Stone"),      TEXT("Stone"),       C::Resource, 100, FLinearColor(0.4f, 0.4f, 0.4f),    TEXT("Mined from boulders.")),
			Item(TEXT("Herb"),       TEXT("Ashroot Herb"),C::Resource, 50,  FLinearColor(0.3f, 0.6f, 0.2f),    TEXT("Medicinal plant. Makes bandages.")),
			Item(TEXT("Berries"),    TEXT("Berries"),     C::Consumable, 30, FLinearColor(0.5f, 0.05f, 0.2f),  TEXT("Eat: a little food and water.")),
			Item(TEXT("ScrapMetal"), TEXT("Scrap Metal"), C::Resource, 100, FLinearColor(0.5f, 0.5f, 0.55f),   TEXT("Salvaged from wrecks and soldiers.")),
			Item(TEXT("RawMeat"),    TEXT("Raw Meat"),    C::Consumable, 20, FLinearColor(0.6f, 0.15f, 0.12f), TEXT("Edible raw, much better cooked at a campfire.")),
			Item(TEXT("AshHide"),    TEXT("Ash Hide"),    C::Resource, 20,  FLinearColor(0.1f, 0.1f, 0.1f),    TEXT("Tough hide of an Ash Stalker. Armor.")),
			Item(TEXT("EmberFang"),  TEXT("Ember Fang"),  C::Resource, 10,  FLinearColor(1.f, 0.4f, 0.05f),    TEXT("Still warm. A rare hunting trophy.")),
			// Consumables
			Item(TEXT("Bandage"),    TEXT("Bandage"),     C::Consumable, 10, FLinearColor(0.9f, 0.9f, 0.85f),  TEXT("Use: heal 30.")),
			Item(TEXT("CookedMeat"), TEXT("Cooked Meat"), C::Consumable, 10, FLinearColor(0.45f, 0.25f, 0.1f), TEXT("Use: restore 45 food.")),
			Item(TEXT("AmmoPack"),   TEXT("Ammo Pack"),   C::Consumable, 10, FLinearColor(0.55f, 0.47f, 0.2f), TEXT("Use: 2 magazines for every gun.")),
			Item(TEXT("ArmorPlate"), TEXT("Hide Armor"),  C::Consumable, 5,  FLinearColor(0.15f, 0.3f, 0.6f),  TEXT("Use: +50 armor.")),
			Item(TEXT("EmberTonic"), TEXT("Ember Tonic"), C::Consumable, 5,  FLinearColor(1.f, 0.5f, 0.1f),    TEXT("Use: full heal and warms you to 37 C.")),
			// Placeables
			Item(TEXT("WoodFoundation"), TEXT("Wood Foundation"), C::Placeable, 20, FLinearColor(0.3f, 0.18f, 0.08f), TEXT("3 x 3 m floor. Snaps to a grid.")),
			Item(TEXT("WoodWall"),       TEXT("Wood Wall"),       C::Placeable, 20, FLinearColor(0.3f, 0.18f, 0.08f), TEXT("Wall on a grid edge.")),
			Item(TEXT("WoodDoorway"),    TEXT("Wood Doorway"),    C::Placeable, 10, FLinearColor(0.3f, 0.18f, 0.08f), TEXT("Wall with a working door.")),
			Item(TEXT("Campfire"),       TEXT("Campfire"),        C::Placeable, 5,  FLinearColor(1.f, 0.45f, 0.1f),   TEXT("Warms you and lets you cook.")),
		};
	}

	TArray<FNARecipe> BuildRecipes()
	{
		auto Recipe = [](const TCHAR* Output, int32 OutputCount, std::initializer_list<FNAIngredient> Inputs, bool bCampfire = false)
		{
			FNARecipe R;
			R.Id = Output;
			R.Output = Output;
			R.OutputCount = OutputCount;
			R.Inputs = Inputs;
			R.bNeedsCampfire = bCampfire;
			return R;
		};
		return {
			Recipe(TEXT("Bandage"),        1, { { TEXT("Herb"), 3 } }),
			Recipe(TEXT("CookedMeat"),     1, { { TEXT("RawMeat"), 1 } }, true),
			Recipe(TEXT("AmmoPack"),       1, { { TEXT("ScrapMetal"), 3 }, { TEXT("Stone"), 1 } }),
			Recipe(TEXT("ArmorPlate"),     1, { { TEXT("AshHide"), 1 }, { TEXT("ScrapMetal"), 2 } }),
			Recipe(TEXT("EmberTonic"),     1, { { TEXT("EmberFang"), 1 }, { TEXT("Herb"), 2 } }, true),
			Recipe(TEXT("Campfire"),       1, { { TEXT("Wood"), 4 }, { TEXT("Stone"), 3 } }),
			Recipe(TEXT("WoodFoundation"), 1, { { TEXT("Wood"), 8 } }),
			Recipe(TEXT("WoodWall"),       1, { { TEXT("Wood"), 5 } }),
			Recipe(TEXT("WoodDoorway"),    1, { { TEXT("Wood"), 6 }, { TEXT("ScrapMetal"), 1 } }),
		};
	}
}

const TArray<FNAItemDef>& NAItems::All()
{
	static const TArray<FNAItemDef> Items = BuildItems();
	return Items;
}

const FNAItemDef* NAItems::Find(FName Id)
{
	return All().FindByPredicate([Id](const FNAItemDef& Def) { return Def.Id == Id; });
}

const TArray<FNARecipe>& NAItems::Recipes()
{
	static const TArray<FNARecipe> List = BuildRecipes();
	return List;
}

const FNARecipe* NAItems::FindRecipe(FName Id)
{
	return Recipes().FindByPredicate([Id](const FNARecipe& Recipe) { return Recipe.Id == Id; });
}

FString NAItems::NameOf(FName Id)
{
	const FNAItemDef* Def = Find(Id);
	return Def ? Def->DisplayName : Id.ToString();
}
