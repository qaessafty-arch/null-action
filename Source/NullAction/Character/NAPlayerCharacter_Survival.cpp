// ANAPlayerCharacter: gathering, inventory menu and base building.

#include "Character/NAPlayerCharacter.h"

#include "Building/NABuildPiece.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAPlayerController.h"
#include "Core/NAPlayerState.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "Inventory/NAInventoryComponent.h"
#include "Inventory/NAItems.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "World/NATerrainManager.h"

namespace NABuildRules
{
	constexpr float Grid = 300.f;
	constexpr float MaxBuildDistance = 900.f;
	constexpr float GatherCooldownSeconds = 0.8f;
}

// ----- Gathering ---------------------------------------------------------------------------------

void ANAPlayerCharacter::UpdateGatherFocus(const FHitResult& Hit)
{
	const UPrimitiveComponent* Component = Hit.GetComponent();
	if (!Component || FVector::Dist(Hit.ImpactPoint, GetActorLocation()) > InteractionRange + 100.f)
	{
		return;
	}
	if (Component->ComponentHasTag(TEXT("NA.Tree")))
	{
		GatherPrompt = TEXT("Chop wood");
	}
	else if (Component->ComponentHasTag(TEXT("NA.Rock")))
	{
		GatherPrompt = TEXT("Mine stone");
	}
	GatherLocation = Hit.ImpactPoint;
}

void ANAPlayerCharacter::ServerGather_Implementation(FVector_NetQuantize Location)
{
	UWorld* World = GetWorld();
	if (!World || IsDead() || !InventoryComponent)
	{
		return;
	}
	// Anti-cheat: range, swing rate, and the server must find a real tree/rock at that spot.
	const float Now = World->GetTimeSeconds();
	if (Now - LastGatherTime < NABuildRules::GatherCooldownSeconds || FVector::Dist(Location, GetActorLocation()) > InteractionRange + 250.f)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	World->OverlapMultiByObjectType(Overlaps, Location, FQuat::Identity, Objects, FCollisionShape::MakeSphere(60.f));

	bool bTree = false;
	bool bRock = false;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (const UPrimitiveComponent* Component = Overlap.GetComponent())
		{
			bTree |= Component->ComponentHasTag(TEXT("NA.Tree"));
			bRock |= Component->ComponentHasTag(TEXT("NA.Rock"));
		}
	}
	if (!bTree && !bRock)
	{
		return;
	}

	LastGatherTime = Now;
	TActorIterator<ANATerrainManager> Terrain(World);
	if (Terrain && !Terrain->TryHarvest(Location))
	{
		if (ANAPlayerController* PC = Cast<ANAPlayerController>(GetController()))
		{
			PC->ClientReceiveChat(ENAChatChannel::System, FString(), bTree ? TEXT("This tree is spent. Find another.") : TEXT("Nothing left to mine here."));
		}
		return;
	}

	if (bTree)
	{
		InventoryComponent->AddItem(NAItems::Wood(), 2);
	}
	else
	{
		InventoryComponent->AddItem(NAItems::Stone(), 2);
		if (FMath::FRand() < 0.25f)
		{
			InventoryComponent->AddItem(NAItems::Scrap(), 1);
		}
	}
}

// ----- Inventory menu ----------------------------------------------------------------------------

int32 ANAPlayerCharacter::GetMenuEntryCount() const
{
	if (InventoryTab == 0)
	{
		return InventoryComponent ? InventoryComponent->GetItems().Num() : 0;
	}
	if (InventoryTab == 2)
	{
		return ANAPlayerState::SkillCount;
	}
	return NAItems::Recipes().Num();
}

void ANAPlayerCharacter::Input_Inventory()
{
	bInventoryOpen = !bInventoryOpen;
	InventoryIndex = 0;
	if (bInventoryOpen)
	{
		StopBuilding();
	}
}

void ANAPlayerCharacter::Input_MenuUp()
{
	const int32 Count = GetMenuEntryCount();
	if (bInventoryOpen && Count > 0)
	{
		InventoryIndex = (InventoryIndex - 1 + Count) % Count;
	}
}

void ANAPlayerCharacter::Input_MenuDown()
{
	const int32 Count = GetMenuEntryCount();
	if (bInventoryOpen && Count > 0)
	{
		InventoryIndex = (InventoryIndex + 1) % Count;
	}
}

void ANAPlayerCharacter::Input_MenuLeft()
{
	if (bInventoryOpen)
	{
		InventoryTab = FMath::Max(0, InventoryTab - 1);
		InventoryIndex = 0;
	}
}

void ANAPlayerCharacter::Input_Skills()
{
	if (bInventoryOpen && InventoryTab == 2)
	{
		bInventoryOpen = false;
		return;
	}
	bInventoryOpen = true;
	InventoryTab = 2;
	InventoryIndex = 0;
	StopBuilding();
}

void ANAPlayerCharacter::Input_MenuRight()
{
	if (bInventoryOpen)
	{
		InventoryTab = FMath::Min(2, InventoryTab + 1);
		InventoryIndex = 0;
	}
}

void ANAPlayerCharacter::Input_MenuConfirm()
{
	if (!bInventoryOpen || !InventoryComponent)
	{
		return;
	}

	if (InventoryTab == 2)
	{
		if (ANAPlayerController* PC = Cast<ANAPlayerController>(GetController()))
		{
			PC->ServerBuySkill(static_cast<uint8>(InventoryIndex)); // The server checks points and ranks.
		}
		return;
	}
	if (InventoryTab == 1)
	{
		const TArray<FNARecipe>& Recipes = NAItems::Recipes();
		if (Recipes.IsValidIndex(InventoryIndex))
		{
			InventoryComponent->RequestCraft(Recipes[InventoryIndex].Id);
		}
		return;
	}

	const TArray<FNAItemStack>& Stacks = InventoryComponent->GetItems();
	if (!Stacks.IsValidIndex(InventoryIndex))
	{
		return;
	}
	const FName ItemId = Stacks[InventoryIndex].ItemId;
	const FNAItemDef* Def = NAItems::Find(ItemId);
	if (Def && Def->Category == ENAItemCategory::Placeable)
	{
		bInventoryOpen = false;
		StartBuilding(ItemId); // Placeables go into build mode.
	}
	else
	{
		InventoryComponent->RequestUse(ItemId);
	}
}

// ----- Building ----------------------------------------------------------------------------------

void ANAPlayerCharacter::StartBuilding(FName ItemId)
{
	if (IsDowned() || CarriedPlayer)
	{
		return;
	}
	ENABuildPieceType Type;
	if (!ANABuildPiece::TypeForItem(ItemId, Type))
	{
		return;
	}
	bBuildMode = true;
	BuildItem = ItemId;
	SetAiming(false);

	if (!BuildPreview)
	{
		BuildPreview = NewObject<UStaticMeshComponent>(this);
		BuildPreview->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		BuildPreview->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
		BuildPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BuildPreview->SetCastShadow(false);
		BuildPreview->SetUsingAbsoluteLocation(true);
		BuildPreview->SetUsingAbsoluteRotation(true);
		BuildPreview->SetUsingAbsoluteScale(true);
		BuildPreview->SetupAttachment(RootComponent);
		BuildPreview->RegisterComponent();
		BuildPreview->CreateAndSetMaterialInstanceDynamic(0);
	}
	BuildPreview->SetVisibility(true);
}

void ANAPlayerCharacter::StopBuilding()
{
	bBuildMode = false;
	BuildItem = NAME_None;
	if (BuildPreview)
	{
		BuildPreview->SetVisibility(false);
	}
}

bool ANAPlayerCharacter::ComputePlacement(ENABuildPieceType Type, const FVector& AimPoint, float Yaw, FTransform& OutTransform) const
{
	const UWorld* World = GetWorld();
	const float Grid = NABuildRules::Grid;
	FVector Base = AimPoint;
	float FinalYaw = Yaw;

	if (Type == ENABuildPieceType::Foundation)
	{
		// Foundations sit on a 3 m grid.
		Base.X = FMath::RoundToFloat(AimPoint.X / Grid) * Grid;
		Base.Y = FMath::RoundToFloat(AimPoint.Y / Grid) * Grid;
		FinalYaw = 0.f;
	}
	else if (Type == ENABuildPieceType::Wall || Type == ENABuildPieceType::Doorway)
	{
		// Walls snap to the nearest edge of the foundation grid cell you're aiming into.
		const float CellX = FMath::RoundToFloat(AimPoint.X / Grid);
		const float CellY = FMath::RoundToFloat(AimPoint.Y / Grid);
		const float FX = (AimPoint.X - (CellX * Grid - Grid * 0.5f)) / Grid; // 0..1 across the cell
		const float FY = (AimPoint.Y - (CellY * Grid - Grid * 0.5f)) / Grid;
		const float ToWest = FX, ToEast = 1.f - FX, ToSouth = FY, ToNorth = 1.f - FY;
		const float Nearest = FMath::Min(FMath::Min(ToWest, ToEast), FMath::Min(ToSouth, ToNorth));
		if (Nearest == ToWest || Nearest == ToEast)
		{
			Base.X = CellX * Grid + (Nearest == ToWest ? -Grid * 0.5f : Grid * 0.5f);
			Base.Y = CellY * Grid;
			FinalYaw = 0.f;   // Wall runs along Y.
		}
		else
		{
			Base.X = CellX * Grid;
			Base.Y = CellY * Grid + (Nearest == ToSouth ? -Grid * 0.5f : Grid * 0.5f);
			FinalYaw = 90.f;  // Wall runs along X.
		}
	}

	// Find what it stands on (ground or a foundation top).
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NABuildPlace), false, this);
	FHitResult Ground;
	if (!World->LineTraceSingleByChannel(Ground, FVector(Base.X, Base.Y, AimPoint.Z + 300.f), FVector(Base.X, Base.Y, AimPoint.Z - 600.f), ECC_Visibility, Params))
	{
		return false;
	}
	Base.Z = Ground.ImpactPoint.Z;
	if (Type == ENABuildPieceType::Foundation)
	{
		Base.Z += 20.f; // Top of the foundation slab sits 20 cm above the ground.
	}
	if ((Type == ENABuildPieceType::Foundation || Type == ENABuildPieceType::Campfire) && Ground.ImpactNormal.Z < 0.7f)
	{
		return false; // Too steep.
	}

	OutTransform = FTransform(FRotator(0.f, FinalYaw, 0.f), Base);

	// Nothing alive may be inside it, and no duplicate in the same spot.
	const FVector Size = ANABuildPiece::SizeOf(Type);
	const FVector Center = Base + FVector(0.f, 0.f, Type == ENABuildPieceType::Foundation ? 0.f : Size.Z * 0.5f + 2.f);
	FCollisionObjectQueryParams Pawns(ECC_Pawn);
	if (World->OverlapAnyTestByObjectType(Center, FRotator(0.f, FinalYaw, 0.f).Quaternion(), Pawns, FCollisionShape::MakeBox(Size * 0.5f - FVector(2.f))))
	{
		return false;
	}
	for (TActorIterator<ANABuildPiece> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->PieceType == Type && FVector::DistSquared(It->GetActorLocation(), Base) < 40.f * 40.f)
		{
			return false;
		}
	}
	return true;
}

void ANAPlayerCharacter::UpdateBuildPreview()
{
	if (!bBuildMode || !BuildPreview)
	{
		return;
	}
	ENABuildPieceType Type;
	if (!ANABuildPiece::TypeForItem(BuildItem, Type) || !InventoryComponent || InventoryComponent->CountOf(BuildItem) <= 0)
	{
		StopBuilding(); // Ran out of pieces.
		return;
	}

	// Aim point: where the camera looks, within building range.
	const FVector Start = FollowCamera->GetComponentLocation();
	const FVector End = Start + FollowCamera->GetForwardVector() * (CameraBoom->TargetArmLength + NABuildRules::MaxBuildDistance);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NABuildAim), false, this);
	FHitResult Hit;
	BuildAimPoint = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) ? Hit.ImpactPoint : End;

	FTransform Placement;
	bPlacementValid = ComputePlacement(Type, BuildAimPoint, BuildYaw, Placement) &&
		FVector::Dist(Placement.GetLocation(), GetActorLocation()) <= NABuildRules::MaxBuildDistance;

	const FVector Size = ANABuildPiece::SizeOf(Type);
	const FVector Center = Placement.GetLocation() + FVector(0.f, 0.f, Type == ENABuildPieceType::Foundation ? 0.f : Size.Z * 0.5f);
	BuildPreview->SetWorldLocationAndRotation(Center, Placement.GetRotation());
	BuildPreview->SetWorldScale3D(Size / 100.f);
	if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(BuildPreview->GetMaterial(0)))
	{
		MID->SetVectorParameterValue(TEXT("Color"), bPlacementValid ? FLinearColor(0.1f, 0.8f, 0.2f) : FLinearColor(0.9f, 0.1f, 0.05f));
	}
}

void ANAPlayerCharacter::ServerPlace_Implementation(FName ItemId, FVector_NetQuantize AimPoint, float Yaw)
{
	ENABuildPieceType Type;
	ANAPlayerState* Builder = GetPlayerState<ANAPlayerState>();
	if (IsDead() || !Builder || !InventoryComponent || !ANABuildPiece::TypeForItem(ItemId, Type) || InventoryComponent->CountOf(ItemId) <= 0)
	{
		return;
	}

	// The server recomputes the snapped transform and validity itself.
	FTransform Placement;
	if (!ComputePlacement(Type, AimPoint, Yaw, Placement) || FVector::Dist(Placement.GetLocation(), GetActorLocation()) > NABuildRules::MaxBuildDistance + 100.f)
	{
		return;
	}

	ANABuildPiece* Piece = GetWorld()->SpawnActorDeferred<ANABuildPiece>(ANABuildPiece::StaticClass(), Placement, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Piece)
	{
		return;
	}
	Piece->PieceType = Type;
	Piece->OwnerName = Builder->GetPlayerName();
	Piece->OwnerId = Builder->GetStableId();
	Piece->FinishSpawning(Placement);
	InventoryComponent->RemoveItem(ItemId, 1);
}
