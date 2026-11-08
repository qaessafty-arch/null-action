#include "Interaction/NAPickup.h"

#include "Character/NACharacterBase.h"
#include "Combat/NAHealthComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Survival/NASurvivalComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/NAWeapon.h"

#define LOCTEXT_NAMESPACE "NullActionPickup"

ANAPickupBase::ANAPickupBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// The interact volume is what the player's look-trace hits. It blocks only the Visibility channel,
	// so it never stops movement or bullets.
	InteractVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractVolume"));
	InteractVolume->InitSphereRadius(55.f);
	InteractVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = InteractVolume;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(InteractVolume);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
	if (Cube.Succeeded()) { ItemMesh->SetStaticMesh(Cube.Object); }
	if (ShapeMaterial.Succeeded()) { ItemMesh->SetMaterial(0, ShapeMaterial.Object); }

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(InteractVolume);
	BaseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BaseMesh->SetRelativeLocation(FVector(0.f, 0.f, -52.f));
	BaseMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.04f));
	if (Cylinder.Succeeded()) { BaseMesh->SetStaticMesh(Cylinder.Object); }
	if (ShapeMaterial.Succeeded()) { BaseMesh->SetMaterial(0, ShapeMaterial.Object); }
}

void ANAPickupBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAPickupBase, bAvailable);
}

void ANAPickupBase::BeginPlay()
{
	Super::BeginPlay();
	RefreshAppearance();
}

void ANAPickupBase::RefreshAppearance()
{
	ItemMesh->SetRelativeScale3D(GetItemSize() / 100.f);
	if (UMaterialInstanceDynamic* MID = ItemMesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), GetItemColor());
	}
	if (UMaterialInstanceDynamic* BaseMID = BaseMesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		BaseMID->SetVectorParameterValue(TEXT("Color"), bAvailable ? FLinearColor(0.75f, 0.08f, 0.05f) : FLinearColor(0.1f, 0.1f, 0.1f));
	}
	ItemMesh->SetVisibility(bAvailable);
}

void ANAPickupBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bAvailable && GetNetMode() != NM_DedicatedServer)
	{
		ItemMesh->AddLocalRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
	}
}

FText ANAPickupBase::GetInteractionPrompt(const ANACharacterBase* User) const
{
	return FText::Format(LOCTEXT("PickUpFormat", "Pick up {0}"), GetItemName());
}

void ANAPickupBase::Interact(ANACharacterBase* User)
{
	if (!HasAuthority() || !bAvailable || !User || User->IsDead())
	{
		return;
	}

	if (GiveTo(User))
	{
		bAvailable = false;
		OnRep_Available();
		if (RespawnSeconds > 0.f)
		{
			GetWorldTimerManager().SetTimer(RespawnTimer, this, &ANAPickupBase::Respawn, RespawnSeconds, false);
		}
	}
}

void ANAPickupBase::Respawn()
{
	bAvailable = true;
	OnRep_Available();
}

void ANAPickupBase::OnRep_Available()
{
	RefreshAppearance();
}

// ---------------------------------------------------------------------------------------------

void ANAWeaponPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAWeaponPickup, WeaponClass);
}

bool ANAWeaponPickup::GiveTo(ANACharacterBase* User)
{
	return WeaponClass && User->GiveWeapon(WeaponClass, /*bEquip*/ true) != nullptr;
}

FText ANAWeaponPickup::GetItemName() const
{
	const ANAWeapon* Defaults = WeaponClass ? WeaponClass->GetDefaultObject<ANAWeapon>() : nullptr;
	return Defaults ? Defaults->GetStats().DisplayName : LOCTEXT("UnknownWeapon", "Weapon");
}

FVector ANAWeaponPickup::GetItemSize() const
{
	const ANAWeapon* Defaults = WeaponClass ? WeaponClass->GetDefaultObject<ANAWeapon>() : nullptr;
	return Defaults ? Defaults->GetStats().ModelSize : FVector(40.f, 6.f, 12.f);
}

// ---------------------------------------------------------------------------------------------

void ANASupplyPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANASupplyPickup, SupplyType);
}

bool ANASupplyPickup::GiveTo(ANACharacterBase* User)
{
	UNAHealthComponent* Health = User->GetHealthComponent();
	UNASurvivalComponent* Survival = User->FindComponentByClass<UNASurvivalComponent>();

	switch (SupplyType)
	{
	case ENASupplyType::Ammo:
	{
		int32 Added = 0;
		for (ANAWeapon* Weapon : User->GetWeapons())
		{
			if (Weapon)
			{
				Added += Weapon->AddReserveAmmo(FMath::RoundToInt(Amount) * Weapon->GetStats().MagazineSize);
			}
		}
		return Added > 0;
	}
	case ENASupplyType::Medkit: return Health && Health->Heal(Amount) > 0.f;
	case ENASupplyType::Armor:  return Health && Health->AddArmor(Amount) > 0.f;
	case ENASupplyType::Food:   return Survival && Survival->RestoreFood(Amount) > 0.f;
	case ENASupplyType::Water:  return Survival && Survival->RestoreWater(Amount) > 0.f;
	}
	return false;
}

FText ANASupplyPickup::GetItemName() const
{
	switch (SupplyType)
	{
	case ENASupplyType::Ammo:   return LOCTEXT("Ammo", "Ammo");
	case ENASupplyType::Medkit: return LOCTEXT("Medkit", "Medkit");
	case ENASupplyType::Armor:  return LOCTEXT("Armor", "Armor Plate");
	case ENASupplyType::Food:   return LOCTEXT("Food", "Food Ration");
	case ENASupplyType::Water:  return LOCTEXT("Water", "Water Bottle");
	}
	return FText::GetEmpty();
}

FLinearColor ANASupplyPickup::GetItemColor() const
{
	switch (SupplyType)
	{
	case ENASupplyType::Ammo:   return FLinearColor(0.55f, 0.47f, 0.2f);
	case ENASupplyType::Medkit: return FLinearColor(0.9f, 0.9f, 0.9f);
	case ENASupplyType::Armor:  return FLinearColor(0.15f, 0.3f, 0.6f);
	case ENASupplyType::Food:   return FLinearColor(0.75f, 0.4f, 0.1f);
	case ENASupplyType::Water:  return FLinearColor(0.1f, 0.6f, 0.8f);
	}
	return FLinearColor::White;
}

#undef LOCTEXT_NAMESPACE
