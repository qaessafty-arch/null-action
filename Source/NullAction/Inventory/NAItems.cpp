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
			// Hunting gear
			Item(TEXT("HuntingBow"),   TEXT("Hunting Bow"),   C::Consumable, 1,  FLinearColor(0.45f, 0.3f, 0.15f), TEXT("Use: equip the silent bow (key 5). +60% damage to creatures.")),
			Item(TEXT("ArrowBundle"),  TEXT("Arrow Bundle"),  C::Consumable, 10, FLinearColor(0.6f, 0.5f, 0.3f),  TEXT("Use: +6 arrows for the bow.")),
			Item(TEXT("HuntingKnife"), TEXT("Hunting Knife"), C::Resource,   1,  FLinearColor(0.7f, 0.7f, 0.75f), TEXT("Carry it to skin creature carcasses (E) for extra hide and meat.")),
			Item(TEXT("SnareTrap"),    TEXT("Snare Trap"),    C::Consumable, 5,  FLinearColor(0.35f, 0.35f, 0.3f), TEXT("Use: set a trap at your feet. Snaps shut on creatures and enemies.")),
			Item(TEXT("Bait"),         TEXT("Meat Bait"),     C::Consumable, 5,  FLinearColor(0.7f, 0.2f, 0.15f), TEXT("Use: drop bait. Hungry creatures nearby come to eat it.")),
			// Boss trophies
			Item(TEXT("BehemothHeart"), TEXT("Behemoth Heart"), C::Resource,   3, FLinearColor(1.f, 0.25f, 0.05f), TEXT("Still burning. Trophy of the Ashen Behemoth. Crafts the Ashen Aegis.")),
			Item(TEXT("AshenAegis"),    TEXT("Ashen Aegis"),    C::Consumable, 3, FLinearColor(0.9f, 0.35f, 0.1f), TEXT("Use: full health, full armor, +1 Ash charge.")),
			// Medicine, tools and protective gear
			Item(TEXT("Antidote"),    TEXT("Antidote"),     C::Consumable, 5, FLinearColor(0.4f, 0.8f, 0.5f),  TEXT("Use: cures poison.")),
			Item(TEXT("Medicine"),    TEXT("Medicine"),     C::Consumable, 5, FLinearColor(0.9f, 0.9f, 1.f),   TEXT("Use: cures infection, heals 20.")),
			Item(TEXT("RepairKit"),   TEXT("Repair Kit"),   C::Consumable, 5, FLinearColor(0.6f, 0.55f, 0.4f), TEXT("Use: fully repairs the weapon in your hands.")),
			Item(TEXT("Binoculars"),  TEXT("Binoculars"),   C::Resource,   1, FLinearColor(0.2f, 0.2f, 0.22f), TEXT("Carry them and press B: zoom and scan creatures (health, weak spot, behaviour).")),
			Item(TEXT("Rebreather"),  TEXT("Rebreather"),   C::Consumable, 1, FLinearColor(0.2f, 0.5f, 0.7f),  TEXT("Use: equip; your breath lasts 4x longer underwater (until you die).")),
			Item(TEXT("FurCoat"),     TEXT("Fur Coat"),     C::Consumable, 1, FLinearColor(0.45f, 0.42f, 0.4f),TEXT("Use: wear it; cold chills you half as fast (until you die).")),
			Item(TEXT("WolfPelt"),    TEXT("Wolf Pelt"),    C::Resource,  20, FLinearColor(0.35f, 0.35f, 0.36f), TEXT("Warm grey fur. Makes Fur Coats.")),
			Item(TEXT("DeerHide"),    TEXT("Deer Hide"),    C::Resource,  20, FLinearColor(0.5f, 0.32f, 0.18f), TEXT("Supple leather. Bandages, rebreather seals.")),
			Item(TEXT("EnergyCell"),  TEXT("Ember Cell"),   C::Resource,  10, FLinearColor(1.f, 0.6f, 0.2f),   TEXT("A rare glowing energy core found in the wild. Used by Engineers and in advanced gear.")),
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
			Recipe(TEXT("HuntingBow"),     1, { { TEXT("Wood"), 6 }, { TEXT("Herb"), 2 } }),
			Recipe(TEXT("ArrowBundle"),    1, { { TEXT("Wood"), 2 }, { TEXT("Stone"), 1 } }),
			Recipe(TEXT("HuntingKnife"),   1, { { TEXT("ScrapMetal"), 2 }, { TEXT("Wood"), 1 } }),
			Recipe(TEXT("SnareTrap"),      1, { { TEXT("ScrapMetal"), 3 }, { TEXT("Wood"), 2 } }),
			Recipe(TEXT("AshenAegis"),     1, { { TEXT("BehemothHeart"), 1 }, { TEXT("AshHide"), 3 }, { TEXT("ScrapMetal"), 4 } }, true),
			Recipe(TEXT("Antidote"),       1, { { TEXT("Herb"), 2 }, { TEXT("Berries"), 1 } }),
			Recipe(TEXT("Medicine"),       1, { { TEXT("Herb"), 4 }, { TEXT("Berries"), 2 } }, true),
			Recipe(TEXT("RepairKit"),      1, { { TEXT("ScrapMetal"), 3 }, { TEXT("Wood"), 1 } }),
			Recipe(TEXT("Binoculars"),     1, { { TEXT("ScrapMetal"), 4 }, { TEXT("Stone"), 2 } }),
			Recipe(TEXT("Rebreather"),     1, { { TEXT("ScrapMetal"), 6 }, { TEXT("DeerHide"), 2 }, { TEXT("EnergyCell"), 1 } }),
			Recipe(TEXT("FurCoat"),        1, { { TEXT("WolfPelt"), 3 }, { TEXT("DeerHide"), 1 } }),
			Recipe(TEXT("Bait"),           1, { { TEXT("RawMeat"), 1 }, { TEXT("Berries"), 2 } }),
			Recipe(TEXT("WoodFoundation"), 1, { { TEXT("Wood"), 8 } }),
			Recipe(TEXT("WoodWall"),       1, { { TEXT("Wood"), 5 } }),
			Recipe(TEXT("WoodDoorway"),    1, { { TEXT("Wood"), 6 }, { TEXT("ScrapMetal"), 1 } }),
		};
	}
}

namespace
{
	TArray<FNAItemDef> BuildItemsWithRarity()
	{
		TArray<FNAItemDef> Items = BuildItems();
		struct FTier { const TCHAR* Id; ENAItemRarity Rarity; };
		const FTier Tiers[] =
		{
			{ TEXT("ScrapMetal"), ENAItemRarity::Uncommon }, { TEXT("AshHide"), ENAItemRarity::Uncommon },
			{ TEXT("WolfPelt"), ENAItemRarity::Uncommon },   { TEXT("DeerHide"), ENAItemRarity::Uncommon },
			{ TEXT("AmmoPack"), ENAItemRarity::Uncommon },   { TEXT("Antidote"), ENAItemRarity::Uncommon },
			{ TEXT("RepairKit"), ENAItemRarity::Uncommon },  { TEXT("HuntingBow"), ENAItemRarity::Uncommon },
			{ TEXT("SnareTrap"), ENAItemRarity::Uncommon },  { TEXT("Binoculars"), ENAItemRarity::Uncommon },
			{ TEXT("EmberFang"), ENAItemRarity::Rare },      { TEXT("EmberTonic"), ENAItemRarity::Rare },
			{ TEXT("ArmorPlate"), ENAItemRarity::Rare },     { TEXT("Medicine"), ENAItemRarity::Rare },
			{ TEXT("FurCoat"), ENAItemRarity::Rare },        { TEXT("EnergyCell"), ENAItemRarity::Rare },
			{ TEXT("Rebreather"), ENAItemRarity::Epic },     { TEXT("AshenAegis"), ENAItemRarity::Epic },
			{ TEXT("BehemothHeart"), ENAItemRarity::Legendary },
		};
		for (FNAItemDef& Def : Items)
		{
			for (const FTier& Tier : Tiers)
			{
				if (Def.Id == FName(Tier.Id)) { Def.Rarity = Tier.Rarity; }
			}
		}
		return Items;
	}
}

const TArray<FNAItemDef>& NAItems::All()
{
	static const TArray<FNAItemDef> Items = BuildItemsWithRarity();
	return Items;
}

FLinearColor NAItems::RarityColor(ENAItemRarity Rarity)
{
	switch (Rarity)
	{
	case ENAItemRarity::Uncommon:  return FLinearColor(0.35f, 0.9f, 0.35f);
	case ENAItemRarity::Rare:      return FLinearColor(0.3f, 0.6f, 1.f);
	case ENAItemRarity::Epic:      return FLinearColor(0.75f, 0.4f, 1.f);
	case ENAItemRarity::Legendary: return FLinearColor(1.f, 0.65f, 0.1f);
	default:                       return FLinearColor(0.9f, 0.9f, 0.9f);
	}
}

const TCHAR* NAItems::RarityName(ENAItemRarity Rarity)
{
	switch (Rarity)
	{
	case ENAItemRarity::Uncommon:  return TEXT("Uncommon");
	case ENAItemRarity::Rare:      return TEXT("Rare");
	case ENAItemRarity::Epic:      return TEXT("Epic");
	case ENAItemRarity::Legendary: return TEXT("Legendary");
	default:                       return TEXT("Common");
	}
}

FName NAItems::RandomItemOfRarity(ENAItemRarity Rarity)
{
	TArray<FName> Pool;
	for (const FNAItemDef& Def : All())
	{
		// Placeables and boss trophies never come from random loot.
		if (Def.Rarity == Rarity && Def.Category != ENAItemCategory::Placeable && Def.Id != FName(TEXT("BehemothHeart")))
		{
			Pool.Add(Def.Id);
		}
	}
	return Pool.Num() > 0 ? Pool[FMath::RandRange(0, Pool.Num() - 1)] : NAME_None;
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
