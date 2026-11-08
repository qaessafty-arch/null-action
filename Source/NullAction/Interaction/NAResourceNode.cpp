#include "Interaction/NAResourceNode.h"

#include "Character/NACharacterBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Inventory/NAInventoryComponent.h"
#include "Inventory/NAItems.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "NullActionResource"

namespace NAResource
{
	UStaticMeshComponent* AddShape(AActor* Owner, USceneComponent* Parent, const TCHAR* Shape, const FVector& Location, const FVector& Size,
		const FLinearColor& Color, bool bTraceTarget)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Shape));
		Part->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Size / 100.f);
		if (bTraceTarget)
		{
			// Lets the look-trace find it without blocking movement or bullets.
			Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Part->SetCollisionResponseToAllChannels(ECR_Ignore);
			Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
		else
		{
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Part->RegisterComponent();
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
		return Part;
	}

	const TCHAR* Sphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* Cylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
}

ANAResourceNode::ANAResourceNode()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ANAResourceNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAResourceNode, NodeType);
	DOREPLIFETIME(ANAResourceNode, bAvailable);
}

void ANAResourceNode::BeginPlay()
{
	Super::BeginPlay();
	using namespace NAResource;
	switch (NodeType)
	{
	case ENAResourceNodeType::HerbBush:
		Parts.Add(AddShape(this, RootComponent, Sphere, FVector(0.f, 0.f, 35.f), FVector(90.f, 90.f, 70.f), FLinearColor(0.12f, 0.3f, 0.08f), true));
		Parts.Add(AddShape(this, RootComponent, Sphere, FVector(20.f, 15.f, 70.f), FVector(25.f), FLinearColor(0.45f, 0.75f, 0.25f), false));
		Parts.Add(AddShape(this, RootComponent, Sphere, FVector(-18.f, -10.f, 65.f), FVector(22.f), FLinearColor(0.45f, 0.75f, 0.25f), false));
		break;
	case ENAResourceNodeType::BerryBush:
		Parts.Add(AddShape(this, RootComponent, Sphere, FVector(0.f, 0.f, 45.f), FVector(110.f, 110.f, 90.f), FLinearColor(0.08f, 0.22f, 0.06f), true));
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float Angle = Index * PI / 3.f;
			Parts.Add(AddShape(this, RootComponent, Sphere, FVector(FMath::Cos(Angle) * 45.f, FMath::Sin(Angle) * 45.f, 50.f + (Index % 2) * 20.f),
				FVector(12.f), FLinearColor(0.55f, 0.03f, 0.15f), false));
		}
		break;
	case ENAResourceNodeType::ScrapPile:
		Parts.Add(AddShape(this, RootComponent, Cube, FVector(0.f, 0.f, 25.f), FVector(140.f, 100.f, 50.f), FLinearColor(0.3f, 0.28f, 0.27f), true));
		Parts.Add(AddShape(this, RootComponent, Cylinder, FVector(30.f, -20.f, 60.f), FVector(30.f, 30.f, 60.f), FLinearColor(0.45f, 0.2f, 0.1f), false));
		Parts.Add(AddShape(this, RootComponent, Cube, FVector(-35.f, 25.f, 60.f), FVector(60.f, 8.f, 40.f), FLinearColor(0.5f, 0.5f, 0.55f), false));
		break;
	}
	OnRep_Available();
}

FText ANAResourceNode::GetInteractionPrompt(const ANACharacterBase* User) const
{
	switch (NodeType)
	{
	case ENAResourceNodeType::HerbBush:  return LOCTEXT("Herbs", "Gather herbs");
	case ENAResourceNodeType::BerryBush: return LOCTEXT("Berries", "Pick berries");
	case ENAResourceNodeType::ScrapPile: return LOCTEXT("Scrap", "Salvage scrap");
	}
	return FText::GetEmpty();
}

void ANAResourceNode::Interact(ANACharacterBase* User)
{
	UNAInventoryComponent* Inventory = User ? User->FindComponentByClass<UNAInventoryComponent>() : nullptr;
	if (!HasAuthority() || !bAvailable || !Inventory)
	{
		return;
	}

	int32 Added = 0;
	switch (NodeType)
	{
	case ENAResourceNodeType::HerbBush:  Added = Inventory->AddItem(NAItems::Herb(), FMath::RandRange(2, 4)); break;
	case ENAResourceNodeType::BerryBush: Added = Inventory->AddItem(NAItems::Berries(), FMath::RandRange(3, 5)); break;
	case ENAResourceNodeType::ScrapPile: Added = Inventory->AddItem(NAItems::Scrap(), FMath::RandRange(2, 4)); break;
	}
	if (Added > 0)
	{
		bAvailable = false;
		OnRep_Available();
		GetWorldTimerManager().SetTimer(RegrowTimer, this, &ANAResourceNode::Regrow, RegrowSeconds, false);
	}
}

void ANAResourceNode::Regrow()
{
	bAvailable = true;
	OnRep_Available();
}

void ANAResourceNode::OnRep_Available()
{
	// Depleted: shrink and hide the produce.
	for (int32 Index = 0; Index < Parts.Num(); ++Index)
	{
		if (Parts[Index])
		{
			Parts[Index]->SetVisibility(bAvailable || Index == 0);
		}
	}
}

// ---------------------------------------------------------------------------------------------------

ANAItemPickup::ANAItemPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	InitialLifeSpan = 180.f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetRelativeScale3D(FVector(0.35f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	if (Cube.Succeeded()) { Mesh->SetStaticMesh(Cube.Object); }
	if (Material.Succeeded()) { Mesh->SetMaterial(0, Material.Object); }
}

void ANAItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAItemPickup, ItemId);
	DOREPLIFETIME(ANAItemPickup, Count);
}

void ANAItemPickup::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		const FNAItemDef* Def = NAItems::Find(ItemId);
		MID->SetVectorParameterValue(TEXT("Color"), Def ? Def->Color : FLinearColor::White);
	}
}

void ANAItemPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() != NM_DedicatedServer)
	{
		Mesh->AddLocalRotation(FRotator(0.f, 120.f * DeltaSeconds, 0.f));
	}
}

FText ANAItemPickup::GetInteractionPrompt(const ANACharacterBase* User) const
{
	return FText::Format(LOCTEXT("TakeItem", "Take {0} x{1}"), FText::FromString(NAItems::NameOf(ItemId)), FText::AsNumber(Count));
}

void ANAItemPickup::Interact(ANACharacterBase* User)
{
	UNAInventoryComponent* Inventory = User ? User->FindComponentByClass<UNAInventoryComponent>() : nullptr;
	if (!HasAuthority() || !Inventory)
	{
		return;
	}
	const int32 Taken = Inventory->AddItem(ItemId, Count);
	Count -= Taken;
	if (Count <= 0)
	{
		Destroy();
	}
}

ANAItemPickup* ANAItemPickup::Drop(UWorld* World, const FVector& Location, FName DroppedItem, int32 DroppedCount)
{
	if (!World || DroppedCount <= 0)
	{
		return nullptr;
	}
	const FTransform Where(FRotator::ZeroRotator, Location + FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), 0.f));
	ANAItemPickup* Pickup = World->SpawnActorDeferred<ANAItemPickup>(ANAItemPickup::StaticClass(), Where, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Pickup)
	{
		Pickup->ItemId = DroppedItem;
		Pickup->Count = DroppedCount;
		Pickup->FinishSpawning(Where);
	}
	return Pickup;
}

#undef LOCTEXT_NAMESPACE
