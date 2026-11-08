#include "Creatures/NADeer.h"

#include "Character/NAPlayerCharacter.h"
#include "Combat/NAHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Interaction/NAResourceNode.h"
#include "Inventory/NAItems.h"
#include "Core/NAAssetSettings.h"

ANADeer::ANADeer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->SetRelativeScale3D(FVector(0.72f));
	HealthComponent->SetMaxHealth(80.f);
	CoatIndex = 4; // Deer brown.
}

bool ANADeer::ShouldSpookFrom(const ANAPlayerCharacter* Player, float Distance) const
{
	// Only a crouched or prone hunter moving slowly can get close.
	return Distance < 1600.f && Player->GetVelocity().Size2D() > 175.f;
}

void ANADeer::DropLoot()
{
	const FVector Side = GetActorRightVector() * 110.f;
	ANAItemPickup::Drop(GetWorld(), GetActorLocation() + Side, NAItems::RawMeat(), 3);
	ANAItemPickup::Drop(GetWorld(), GetActorLocation() - Side, TEXT("DeerHide"), 2);
}

const FNAModelSlot& ANADeer::GetModelSlot() const
{
	return UNAAssetSettings::Get()->Deer;
}
