#include "Interaction/NALootContainer.h"

#include "Character/NACharacterBase.h"
#include "Combat/NAHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAPlayerController.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Survival/NASurvivalComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/NAWeapon.h"

#define LOCTEXT_NAMESPACE "NullActionLoot"

ANALootContainer::ANALootContainer()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Box = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Box"));
	RootComponent = Box;
	Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Box->SetRelativeScale3D(FVector(0.9f, 0.6f, 0.55f));

	Lid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lid"));
	Lid->SetupAttachment(Box);
	Lid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Lid->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
	Lid->SetRelativeScale3D(FVector(1.05f, 1.05f, 0.12f));

	if (Cube.Succeeded()) { Box->SetStaticMesh(Cube.Object); Lid->SetStaticMesh(Cube.Object); }
	if (ShapeMaterial.Succeeded()) { Box->SetMaterial(0, ShapeMaterial.Object); Lid->SetMaterial(0, ShapeMaterial.Object); }
}

void ANALootContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANALootContainer, bOpened);
}

void ANALootContainer::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInstanceDynamic* MID = Box->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.18f, 0.22f, 0.1f));
	}
	if (UMaterialInstanceDynamic* LidMID = Lid->CreateAndSetMaterialInstanceDynamic(0))
	{
		LidMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.15f, 0.07f));
	}
	OnRep_Opened();
}

FText ANALootContainer::GetInteractionPrompt(const ANACharacterBase* User) const
{
	return LOCTEXT("Search", "Search crate");
}

void ANALootContainer::Interact(ANACharacterBase* User)
{
	if (!HasAuthority() || bOpened || !User)
	{
		return;
	}

	// Server rolls the loot: ammo for every gun, plus one random supply.
	TArray<FString> Found;
	for (ANAWeapon* Weapon : User->GetWeapons())
	{
		if (Weapon && Weapon->AddReserveAmmo(Weapon->GetStats().MagazineSize * 2) > 0)
		{
			Found.AddUnique(TEXT("ammo"));
		}
	}

	UNAHealthComponent* Health = User->GetHealthComponent();
	UNASurvivalComponent* Survival = User->FindComponentByClass<UNASurvivalComponent>();
	switch (FMath::RandRange(0, 3))
	{
	case 0: if (Health && Health->Heal(40.f) > 0.f) { Found.Add(TEXT("a medkit")); } break;
	case 1: if (Health && Health->AddArmor(40.f) > 0.f) { Found.Add(TEXT("armor")); } break;
	case 2: if (Survival && Survival->RestoreFood(40.f) > 0.f) { Found.Add(TEXT("food")); } break;
	default: if (Survival && Survival->RestoreWater(40.f) > 0.f) { Found.Add(TEXT("water")); } break;
	}

	if (ANAPlayerController* Player = Cast<ANAPlayerController>(User->GetController()))
	{
		Player->ClientReceiveChat(ENAChatChannel::System, FString(),
			Found.Num() > 0 ? FString::Printf(TEXT("You found %s."), *FString::Join(Found, TEXT(", "))) : TEXT("The crate is empty of anything you need."));
	}

	bOpened = true;
	OnRep_Opened();
	GetWorldTimerManager().SetTimer(RefillTimer, this, &ANALootContainer::Refill, RefillSeconds, false);
}

void ANALootContainer::Refill()
{
	bOpened = false;
	OnRep_Opened();
}

void ANALootContainer::OnRep_Opened()
{
	// Open crates show their lid propped back.
	Lid->SetRelativeLocationAndRotation(bOpened ? FVector(-55.f, 0.f, 85.f) : FVector(0.f, 0.f, 55.f), bOpened ? FRotator(70.f, 0.f, 0.f) : FRotator::ZeroRotator);
}

#undef LOCTEXT_NAMESPACE
