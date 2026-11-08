#include "Creatures/NAWolf.h"

#include "Combat/NAHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Interaction/NAResourceNode.h"
#include "Inventory/NAItems.h"
#include "Core/NAAssetSettings.h"

ANAWolf::ANAWolf(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Same skeleton-of-shapes as the Ash Stalker, scaled down to wolf size, grey fur and pale eyes.
	GetCapsuleComponent()->SetRelativeScale3D(FVector(0.72f));
	HealthComponent->SetMaxHealth(90.f);
	BiteDamage = 14.f;
	BiteRange = 190.f;
	BiteCooldownSeconds = 1.1f;
	BodyColor = FLinearColor(0.22f, 0.22f, 0.23f);
	GlowColor = FLinearColor(0.85f, 0.8f, 0.45f);
	SpawnGroup = 1;
}

FName ANAWolf::GetSkinItem() const
{
	return TEXT("WolfPelt");
}

void ANAWolf::DropLoot()
{
	const FVector Side = GetActorRightVector() * 120.f;
	ANAItemPickup::Drop(GetWorld(), GetActorLocation() + Side, TEXT("WolfPelt"), 1);
	ANAItemPickup::Drop(GetWorld(), GetActorLocation() - Side, NAItems::RawMeat(), 2);
}

const FNAModelSlot& ANAWolf::GetModelSlot() const
{
	return UNAAssetSettings::Get()->Wolf;
}
