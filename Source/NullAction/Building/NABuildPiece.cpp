#include "Building/NABuildPiece.h"

#include "Character/NACharacterBase.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAPlayerState.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interaction/NADoor.h"
#include "Inventory/NAInventoryComponent.h"
#include "Inventory/NAItems.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "NullActionBuild"

namespace NABuild
{
	const FLinearColor Planks(0.28f, 0.17f, 0.08f);
	const FLinearColor Beams(0.18f, 0.1f, 0.05f);
	const FLinearColor StoneRing(0.3f, 0.29f, 0.28f);
	const FLinearColor Embers(1.f, 0.35f, 0.05f);
}

ANABuildPiece::ANABuildPiece()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ANABuildPiece::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANABuildPiece, PieceType);
	DOREPLIFETIME(ANABuildPiece, OwnerName);
	DOREPLIFETIME(ANABuildPiece, Health);
}

FName ANABuildPiece::ItemFor(ENABuildPieceType Type)
{
	switch (Type)
	{
	case ENABuildPieceType::Foundation: return NAItems::Foundation();
	case ENABuildPieceType::Wall:       return NAItems::Wall();
	case ENABuildPieceType::Doorway:    return NAItems::Doorway();
	case ENABuildPieceType::Campfire:   return NAItems::Campfire();
	}
	return NAME_None;
}

bool ANABuildPiece::TypeForItem(FName ItemId, ENABuildPieceType& OutType)
{
	const ENABuildPieceType All[] = { ENABuildPieceType::Foundation, ENABuildPieceType::Wall, ENABuildPieceType::Doorway, ENABuildPieceType::Campfire };
	for (const ENABuildPieceType Type : All)
	{
		if (ItemFor(Type) == ItemId)
		{
			OutType = Type;
			return true;
		}
	}
	return false;
}

FVector ANABuildPiece::SizeOf(ENABuildPieceType Type)
{
	switch (Type)
	{
	case ENABuildPieceType::Foundation: return FVector(300.f, 300.f, 40.f);
	case ENABuildPieceType::Wall:
	case ENABuildPieceType::Doorway:    return FVector(20.f, 300.f, 280.f);
	case ENABuildPieceType::Campfire:   return FVector(100.f, 100.f, 40.f);
	}
	return FVector(100.f);
}

ANABuildPiece* ANABuildPiece::FindCampfireNear(const UWorld* World, const FVector& Location, float Radius)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ANABuildPiece> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->IsCampfire() && FVector::DistSquared(It->GetActorLocation(), Location) <= Radius * Radius)
		{
			return *It;
		}
	}
	return nullptr;
}

UStaticMeshComponent* ANABuildPiece::AddPart(const FVector& BottomCentre, const FVector& Size, const FLinearColor& Color, bool bCollision)
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetupAttachment(RootComponent);
	Part->SetStaticMesh(Cube);
	Part->SetMaterial(0, Material);
	Part->SetRelativeLocation(BottomCentre + FVector(0.f, 0.f, Size.Z * 0.5f));
	Part->SetRelativeScale3D(Size / 100.f);
	if (bCollision)
	{
		Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
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
	Parts.Add(Part);
	return Part;
}

void ANABuildPiece::BuildShape()
{
	using namespace NABuild;
	switch (PieceType)
	{
	case ENABuildPieceType::Foundation:
		AddPart(FVector(0.f, 0.f, -20.f), FVector(300.f, 300.f, 40.f), Planks);
		// Corner posts going into the ground so it looks anchored on slopes.
		for (int32 Corner = 0; Corner < 4; ++Corner)
		{
			const FVector Offset((Corner & 1) ? 135.f : -135.f, (Corner & 2) ? 135.f : -135.f, -120.f);
			AddPart(Offset, FVector(25.f, 25.f, 100.f), Beams);
		}
		break;

	case ENABuildPieceType::Wall:
		AddPart(FVector::ZeroVector, FVector(20.f, 300.f, 280.f), Planks);
		AddPart(FVector(0.f, -145.f, 0.f), FVector(26.f, 14.f, 280.f), Beams);
		AddPart(FVector(0.f, 145.f, 0.f), FVector(26.f, 14.f, 280.f), Beams);
		break;

	case ENABuildPieceType::Doorway:
		// Opening 1.1 m wide, 2.2 m high in the middle.
		AddPart(FVector(0.f, -102.5f, 0.f), FVector(20.f, 95.f, 280.f), Planks);
		AddPart(FVector(0.f, 102.5f, 0.f), FVector(20.f, 95.f, 280.f), Planks);
		AddPart(FVector(0.f, 0.f, 220.f), FVector(20.f, 110.f, 60.f), Planks);
		AddPart(FVector(0.f, -57.f, 0.f), FVector(26.f, 8.f, 230.f), Beams);
		AddPart(FVector(0.f, 57.f, 0.f), FVector(26.f, 8.f, 230.f), Beams);
		break;

	case ENABuildPieceType::Campfire:
		for (int32 Stone = 0; Stone < 8; ++Stone)
		{
			const float Angle = Stone * PI / 4.f;
			AddPart(FVector(FMath::Cos(Angle) * 45.f, FMath::Sin(Angle) * 45.f, 0.f), FVector(22.f, 22.f, 18.f), StoneRing, true); // Low enough to step over; lets you aim at it.
		}
		AddPart(FVector::ZeroVector, FVector(50.f, 12.f, 12.f), Beams, false);
		AddPart(FVector::ZeroVector, FVector(12.f, 50.f, 12.f), Beams, false);
		AddPart(FVector(0.f, 0.f, 10.f), FVector(30.f, 30.f, 20.f), Embers, false);
		FireLight = NewObject<UPointLightComponent>(this);
		FireLight->SetupAttachment(RootComponent);
		FireLight->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
		FireLight->SetIntensity(6000.f);
		FireLight->SetAttenuationRadius(800.f);
		FireLight->SetLightColor(FLinearColor(1.f, 0.55f, 0.2f));
		FireLight->RegisterComponent();
		break;
	}
}

void ANABuildPiece::BeginPlay()
{
	Super::BeginPlay();
	BuildShape(); // Runs on every machine once the type has replicated.

	// Doorways get a real, replicated door (server spawns it).
	if (HasAuthority() && PieceType == ENABuildPieceType::Doorway)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FTransform Hinge = FTransform(FRotator::ZeroRotator, FVector(0.f, -55.f, 0.f)) * GetActorTransform();
		SpawnedDoor = GetWorld()->SpawnActor<ANADoor>(ANADoor::StaticClass(), Hinge, Params);
	}
}

void ANABuildPiece::Destroyed()
{
	if (HasAuthority() && SpawnedDoor)
	{
		SpawnedDoor->Destroy();
	}
	Super::Destroyed();
}

void ANABuildPiece::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (FireLight)
	{
		FlickerTime += DeltaSeconds;
		FireLight->SetIntensity(5000.f + FMath::Sin(FlickerTime * 13.f) * 700.f + FMath::Sin(FlickerTime * 7.3f) * 500.f);
	}
}

float ANABuildPiece::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (!HasAuthority() || Damage <= 0.f)
	{
		return 0.f;
	}
	Health -= Damage;
	if (Health <= 0.f)
	{
		Destroy();
	}
	return Damage;
}

FText ANABuildPiece::GetInteractionPrompt(const ANACharacterBase* User) const
{
	return FText::Format(LOCTEXT("Demolish", "Demolish {0} (refund half)"), FText::FromString(NAItems::NameOf(ItemFor(PieceType))));
}

bool ANABuildPiece::CanInteract(const ANACharacterBase* User) const
{
	// Only the builder sees the demolish option (the server re-checks by account id).
	const ANAPlayerState* State = User ? User->GetPlayerState<ANAPlayerState>() : nullptr;
	return State && State->GetPlayerName() == OwnerName;
}

void ANABuildPiece::Interact(ANACharacterBase* User)
{
	const ANAPlayerState* State = User ? User->GetPlayerState<ANAPlayerState>() : nullptr;
	if (!HasAuthority() || !State || State->GetStableId() != OwnerId)
	{
		return; // Not yours.
	}
	if (UNAInventoryComponent* Inventory = User->FindComponentByClass<UNAInventoryComponent>())
	{
		if (const FNARecipe* Recipe = NAItems::FindRecipe(ItemFor(PieceType)))
		{
			for (const FNAIngredient& Input : Recipe->Inputs)
			{
				Inventory->AddItem(Input.Item, FMath::Max(1, Input.Count / 2));
			}
		}
	}
	Destroy();
}

#undef LOCTEXT_NAMESPACE
