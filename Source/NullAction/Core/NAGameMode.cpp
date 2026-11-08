#include "Core/NAGameMode.h"

#include "NullAction.h"
#include "Character/NAPlayerCharacter.h"
#include "Core/NAHUD.h"
#include "EngineUtils.h"
#include "AI/NAEnemyAIController.h"
#include "AI/NAEnemySoldier.h"
#include "Combat/NATrainingDummy.h"
#include "Core/NAPlayerController.h"
#include "Core/NAGameState.h"
#include "Creatures/NACreature.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Interaction/NAPickup.h"
#include "TimerManager.h"
#include "Weapons/NAWeaponTypes.h"
#include "World/NATestArena.h"
#include "World/NAWaterVolume.h"
#include "World/NAWorldEnvironment.h"
#include "World/NATerrainManager.h"
#include "World/NACabin.h"
#include "Interaction/NADoor.h"
#include "Interaction/NALootContainer.h"
#include "Interaction/NANote.h"
#include "Interaction/NAResourceNode.h"
#include "Interaction/NAAshShrine.h"
#include "Creatures/NABoss.h"
#include "Creatures/NAHorse.h"
#include "Creatures/NAWolf.h"
#include "Creatures/NADeer.h"
#include "Creatures/NASkyMount.h"
#include "Core/NAPlayerState.h"
#include "Combat/NAHealthComponent.h"
#include "GameFramework/GameStateBase.h"

ANAGameMode::ANAGameMode()
{
	DefaultPawnClass = ANAPlayerCharacter::StaticClass();
	HUDClass = ANAHUD::StaticClass();
	PlayerControllerClass = ANAPlayerController::StaticClass();
	PlayerStateClass = ANAPlayerState::StaticClass();
	GameStateClass = ANAGameState::StaticClass();
	TestArenaClass = ANATestArena::StaticClass();
}

void ANAGameMode::StartPlay()
{
	EnsureTestArenaIfLevelIsEmpty();

	// One shared sky, time of day and weather for everyone on this server.
	if (!ANAWorldEnvironment::Get(this))
	{
		GetWorld()->SpawnActor<ANAWorldEnvironment>(ANAWorldEnvironment::StaticClass(), FTransform::Identity);
	}
	Super::StartPlay();
}

AActor* ANAGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Players can be placed before StartPlay (e.g. PIE), so make sure starts exist first.
	EnsureTestArenaIfLevelIsEmpty();

	// In arena mode, always use the arena's spawn ring. Otherwise the base class would prefer the
	// editor's temporary "play from camera" start, which can be in mid-air outside the arena.
	if (ArenaStarts.Num() > 0)
	{
		APlayerStart* Start = ArenaStarts[NextArenaStart % ArenaStarts.Num()];
		++NextArenaStart;
		if (Start)
		{
			return Start;
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void ANAGameMode::EnsureTestArenaIfLevelIsEmpty()
{
	if (bCheckedLevel || !bBuildTestArenaInEmptyLevels)
	{
		return;
	}
	bCheckedLevel = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		// The editor adds a temporary "PlayerStartPIE" when you press Play in a level with no
		// spawn points. That doesn't make it a real level, so ignore it. Matched by class name
		// because that class's header isn't public in every engine version.
		static const FName PIEStartClassName(TEXT("PlayerStartPIE"));
		if (It->GetClass()->GetFName() != PIEStartClassName)
		{
			return; // A real level with its own spawn points: leave it alone.
		}
	}

	// Reuse a hand-placed arena if there is one.
	if (TActorIterator<ANATestArena> It(World); It)
	{
		TestArena = *It;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (!TestArena)
	{
		UClass* ArenaClass = TestArenaClass ? TestArenaClass.Get() : ANATestArena::StaticClass();
		TestArena = World->SpawnActor<ANATestArena>(ArenaClass, FTransform::Identity, Params);
	}

	if (!TestArena)
	{
		UE_LOG(LogNullAction, Error, TEXT("Failed to spawn the test arena."));
		return;
	}

	for (int32 Index = 0; Index < TestArenaPlayerStarts; ++Index)
	{
		if (APlayerStart* Start = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), TestArena->GetSpawnTransform(Index), Params))
		{
			ArenaStarts.Add(Start);
		}
	}

	SpawnArenaContent();

	UE_LOG(LogNullAction, Log, TEXT("Level has no PlayerStarts: built test arena with %d spawn points."), TestArenaPlayerStarts);
}

void ANAGameMode::SpawnArenaContent()
{
	UWorld* World = GetWorld();
	if (!World || !TestArena)
	{
		return;
	}

	const FTransform ArenaTransform = TestArena->GetActorTransform();
	auto At = [&ArenaTransform](const FVector& Local, float Yaw = 0.f)
	{
		return FTransform(ArenaTransform.GetRotation() * FRotator(0.f, Yaw, 0.f).Quaternion(), ArenaTransform.TransformPosition(Local));
	};

	// Weapon rack in front of the spawn ring (the pistol is already in every player's hands).
	struct FRackEntry { UClass* WeaponClass; FVector Location; };
	const FRackEntry WeaponRack[] =
	{
		{ ANAWeapon_AssaultRifle::StaticClass(), FVector(500.f, -450.f, 55.f) },
		{ ANAWeapon_Shotgun::StaticClass(),      FVector(500.f, -150.f, 55.f) },
		{ ANAWeapon_SniperRifle::StaticClass(),  FVector(500.f,  150.f, 55.f) },
		{ ANAWeapon_Pistol::StaticClass(),       FVector(500.f,  450.f, 55.f) },
	};
	for (const FRackEntry& Entry : WeaponRack)
	{
		const FTransform Where = At(Entry.Location);
		if (ANAWeaponPickup* Pickup = World->SpawnActorDeferred<ANAWeaponPickup>(ANAWeaponPickup::StaticClass(), Where,
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Pickup->WeaponClass = Entry.WeaponClass;
			Pickup->FinishSpawning(Where);
		}
	}

	// Supplies behind the spawn ring.
	struct FSupplyEntry { ENASupplyType Type; float Amount; };
	const FSupplyEntry Supplies[] =
	{
		{ ENASupplyType::Ammo,   2.f },
		{ ENASupplyType::Medkit, 50.f },
		{ ENASupplyType::Armor,  50.f },
		{ ENASupplyType::Food,   40.f },
		{ ENASupplyType::Water,  40.f },
	};
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Supplies)); ++Index)
	{
		const FTransform Where = At(FVector(-450.f, -600.f + 300.f * Index, 55.f));
		if (ANASupplyPickup* Pickup = World->SpawnActorDeferred<ANASupplyPickup>(ANASupplyPickup::StaticClass(), Where,
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Pickup->SupplyType = Supplies[Index].Type;
			Pickup->Amount = Supplies[Index].Amount;
			Pickup->FinishSpawning(Where);
		}
	}

	// Shooting range on the open +Y side of spawn (turn left from spawn), clear of the movement lanes.
	// Near row at 22 m, far row at 34 m. Dummies face back toward spawn.
	const FVector DummySpots[] =
	{
		FVector( 300.f, 2200.f, 100.f),
		FVector( 900.f, 2200.f, 100.f),
		FVector(1500.f, 2200.f, 100.f),
		FVector( 600.f, 3400.f, 100.f),
		FVector(1200.f, 3400.f, 100.f),
	};
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (const FVector& Spot : DummySpots)
	{
		World->SpawnActor<ANATrainingDummy>(ANATrainingDummy::StaticClass(), At(Spot, -90.f), Params);
	}

	// Combat yard (to your right from spawn, ~30 m away): three soldiers on overlapping patrol routes.
	auto Route = [&ArenaTransform](std::initializer_list<FVector> Points)
	{
		TArray<FVector> WorldPoints;
		for (const FVector& Point : Points)
		{
			WorldPoints.Add(ArenaTransform.TransformPosition(Point));
		}
		return WorldPoints;
	};
	EnemyPosts.Reset();
	EnemyPosts.Add({ At(FVector(2500.f, -2000.f, 100.f), 180.f), Route({ FVector(2500.f, -2000.f, 100.f), FVector(3300.f, -2000.f, 100.f), FVector(3300.f, -1600.f, 100.f) }) });
	EnemyPosts.Add({ At(FVector(3300.f, -3200.f, 100.f), 135.f), Route({ FVector(3300.f, -3200.f, 100.f), FVector(2600.f, -3200.f, 100.f), FVector(2800.f, -2700.f, 100.f) }) });
	EnemyPosts.Add({ At(FVector(2200.f, -2700.f, 100.f), 90.f),  Route({ FVector(2200.f, -2700.f, 100.f), FVector(2200.f, -3400.f, 100.f), FVector(3200.f, -3550.f, 100.f), FVector(3000.f, -2300.f, 100.f) }) });

	for (int32 Index = 0; Index < EnemyPosts.Num(); ++Index)
	{
		SpawnEnemy(Index);
	}

	// Water for the diving pool (back-right corner). Surface at 2.8 m, matching the pool walls.
	World->SpawnActor<ANAWaterVolume>(ANAWaterVolume::StaticClass(), At(FVector(-2600.f, -2600.f, 140.f)), Params);

	// ----- Open world beyond the arena walls (vault over a wall with Space) -----
	World->SpawnActor<ANATerrainManager>(ANATerrainManager::StaticClass(), FTransform::Identity, Params);

	// Mirror Lake (north-west): surface at the terrain's water level.
	{
		const FVector2D Lake = NATerrain::LakeCenter();
		const float Surface = NATerrain::WaterLevel();
		const FTransform LakeTransform(FRotator::ZeroRotator, FVector(Lake.X, Lake.Y, Surface - 400.f));
		if (ANAWaterVolume* LakeWater = World->SpawnActorDeferred<ANAWaterVolume>(ANAWaterVolume::StaticClass(), LakeTransform,
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			LakeWater->SetExtent(FVector(9000.f, 9000.f, 400.f));
			LakeWater->FinishSpawning(LakeTransform);
		}
	}

	// The Old Cabin, just outside the east wall, with its door, crate, note and medkit.
	{
		const FVector2D CabinXY = NATerrain::CabinCenter();
		const FTransform CabinTransform(FRotator(0.f, 180.f, 0.f), FVector(CabinXY.X, CabinXY.Y, 0.f)); // Front door faces the arena.
		if (ANACabin* Cabin = World->SpawnActor<ANACabin>(ANACabin::StaticClass(), CabinTransform, Params))
		{
			World->SpawnActor<ANADoor>(ANADoor::StaticClass(), Cabin->FrontDoorTransform(), Params);
			if (ANADoor* StorageDoor = World->SpawnActorDeferred<ANADoor>(ANADoor::StaticClass(), Cabin->StorageDoorTransform(),
				nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
			{
				StorageDoor->Width = 100.f;
				StorageDoor->FinishSpawning(Cabin->StorageDoorTransform());
			}
			World->SpawnActor<ANALootContainer>(ANALootContainer::StaticClass(), Cabin->CrateTransform(), Params);

			if (ANANote* Note = World->SpawnActorDeferred<ANANote>(ANANote::StaticClass(), Cabin->NoteTransform(),
				nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
			{
				Note->Title = TEXT("Journal of M. Varga, field technician, Site NULL");
				Note->Lines = {
					TEXT("Day 41. The Array ran a full cycle last night. Every light in the valley went out at once. Then the sky turned the colour of ash."),
					TEXT("Day 43. Pellan walked back in from the treeline. We buried Pellan on Day 38."),
					TEXT("He doesn't remember dying. There's grey dust in the creases of his skin that won't wash off. He calls it the Rebirth."),
					TEXT("Day 47. Something is hunting near the lake at night. Long, low, eyes like coals. It isn't on any list."),
					TEXT("If you're reading this: the facility is north, past the peaks. Don't trust anyone who's come back."),
				};
				Note->FinishSpawning(Cabin->NoteTransform());
			}

			if (ANASupplyPickup* Medkit = World->SpawnActorDeferred<ANASupplyPickup>(ANASupplyPickup::StaticClass(), Cabin->MedkitTransform(),
				nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
			{
				Medkit->SupplyType = ENASupplyType::Medkit;
				Medkit->Amount = 60.f;
				Medkit->FinishSpawning(Cabin->MedkitTransform());
			}
		}
	}

	// Gathering spots: herbs and berries around the cabin and arena edges, scrap near the combat yard.
	{
		struct FNodeSpot { ENAResourceNodeType Type; FVector Location; };
		const FVector2D Cabin = NATerrain::CabinCenter();
		const FNodeSpot Spots[] =
		{
			{ ENAResourceNodeType::HerbBush,  FVector(Cabin.X + 700.f, Cabin.Y + 600.f, 0.f) },
			{ ENAResourceNodeType::HerbBush,  FVector(Cabin.X - 800.f, Cabin.Y - 700.f, 0.f) },
			{ ENAResourceNodeType::HerbBush,  FVector(-3500.f, -500.f, 0.f) },
			{ ENAResourceNodeType::HerbBush,  FVector(-1200.f, 3600.f, 0.f) },
			{ ENAResourceNodeType::BerryBush, FVector(Cabin.X + 900.f, Cabin.Y - 300.f, 0.f) },
			{ ENAResourceNodeType::BerryBush, FVector(-3600.f, 1200.f, 0.f) },
			{ ENAResourceNodeType::BerryBush, FVector(2000.f, 3600.f, 0.f) },
			{ ENAResourceNodeType::ScrapPile, FVector(1900.f, -1800.f, 0.f) },
			{ ENAResourceNodeType::ScrapPile, FVector(3700.f, -3700.f, 0.f) },
			{ ENAResourceNodeType::ScrapPile, FVector(Cabin.X - 300.f, Cabin.Y + 900.f, 0.f) },
		};
		for (const FNodeSpot& Spot : Spots)
		{
			const FVector Ground(Spot.Location.X, Spot.Location.Y, FMath::Max(0.f, NATerrain::HeightAt(Spot.Location.X, Spot.Location.Y)));
			const FTransform Where(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), ArenaTransform.TransformPosition(Ground));
			if (ANAResourceNode* Node = World->SpawnActorDeferred<ANAResourceNode>(ANAResourceNode::StaticClass(), Where,
				nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
			{
				Node->NodeType = Spot.Type;
				Node->FinishSpawning(Where);
			}
		}
	}

	// Ash Stalker den in the far back-left corner (behind spawn, to the left).
	CreatureDenLocation = ArenaTransform.TransformPosition(FVector(-2800.f, 2800.f, 80.f));
	SpawnCreature();

	SpawnShrines(ArenaTransform);

	// The Ash Crater, east of the arena and north of the cabin: the Ashen Behemoth's lair.
	{
		const FVector2D Crater = NATerrain::CraterCenter();
		BossLairLocation = FVector(Crater.X, Crater.Y, NATerrain::HeightAt(Crater.X, Crater.Y) + 220.f);
		SpawnBoss();
	}

	// A wild horse herd grazing on the open grassland west of the arena.
	{
		const FVector2D Herd(2500.f, -9500.f);
		HorseHerdCenter = FVector(Herd.X, Herd.Y, NATerrain::HeightAt(Herd.X, Herd.Y) + 120.f);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			SpawnWildHorse();
		}
	}

	// Whispering Pines (south-west): a wolf pack's den, and deer grazing nearby.
	{
		const FVector2D Den(-11000.f, -8000.f);
		WolfDenLocation = FVector(Den.X, Den.Y, NATerrain::HeightAt(Den.X, Den.Y) + 60.f);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			SpawnWolf();
		}
		const FVector2D Herd(-7000.f, -3500.f);
		DeerHerdCenter = FVector(Herd.X, Herd.Y, NATerrain::HeightAt(Herd.X, Herd.Y) + 90.f);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			SpawnDeer();
		}
	}

	// Ash Glider roost on the Grey Peaks foothills (north-east).
	{
		const FVector2D Roost(14500.f, -4000.f);
		SkyRoostLocation = FVector(Roost.X, Roost.Y, NATerrain::HeightAt(Roost.X, Roost.Y) + 120.f);
		SpawnSkyMount();
		SpawnSkyMount();
	}

	ScheduleNextWorldEvent(240.f); // First world event a few minutes in.
	SpawnShipwreck();
}

void ANAGameMode::SpawnBoss()
{
	const FTransform Where(FRotator(0.f, 200.f, 0.f), BossLairLocation);
	if (ANABoss* Boss = GetWorld()->SpawnActorDeferred<ANABoss>(ANABoss::StaticClass(), Where, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
	{
		Boss->SetLairLocation(BossLairLocation);
		Boss->FinishSpawning(Where);
	}
}

void ANAGameMode::SpawnWildHorse()
{
	const FVector2D Offset = FMath::RandPointInCircle(900.f);
	const FVector Where(HorseHerdCenter.X + Offset.X, HorseHerdCenter.Y + Offset.Y,
		NATerrain::HeightAt(HorseHerdCenter.X + Offset.X, HorseHerdCenter.Y + Offset.Y) + 120.f);
	const FTransform Spawn(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Where);
	if (ANAHorse* Horse = GetWorld()->SpawnActorDeferred<ANAHorse>(ANAHorse::StaticClass(), Spawn, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
	{
		Horse->SetHerdCenter(HorseHerdCenter);
		Horse->SetCoat(static_cast<uint8>(FMath::RandRange(0, 3)));
		Horse->FinishSpawning(Spawn);
	}
}

void ANAGameMode::OnHorseKilled(ANAHorse* Horse, AController* Killer)
{
	FTimerHandle Handle;
	if (Horse && Horse->IsA<ANASkyMount>())
	{
		GetWorldTimerManager().SetTimer(Handle, this, &ANAGameMode::SpawnSkyMount, 400.f, false);
		return;
	}
	if (Horse && Horse->IsA<ANADeer>())
	{
		AwardXP(Killer, 20, TEXT("Deer"));
		GetWorldTimerManager().SetTimer(Handle, this, &ANAGameMode::SpawnDeer, 180.f, false);
		return;
	}
	GetWorldTimerManager().SetTimer(Handle, this, &ANAGameMode::SpawnWildHorse, 300.f, false);
}

void ANAGameMode::SpawnSkyMount()
{
	const FVector2D Offset = FMath::RandPointInCircle(500.f);
	const float X = SkyRoostLocation.X + Offset.X;
	const float Y = SkyRoostLocation.Y + Offset.Y;
	const FTransform Where(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), FVector(X, Y, NATerrain::HeightAt(X, Y) + 140.f));
	if (ANASkyMount* Glider = GetWorld()->SpawnActorDeferred<ANASkyMount>(ANASkyMount::StaticClass(), Where, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
	{
		Glider->SetHerdCenter(SkyRoostLocation);
		Glider->FinishSpawning(Where);
	}
}

void ANAGameMode::SpawnWolf()
{
	const FVector2D Offset = FMath::RandPointInCircle(600.f);
	const float X = WolfDenLocation.X + Offset.X;
	const float Y = WolfDenLocation.Y + Offset.Y;
	const FTransform Where(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), FVector(X, Y, NATerrain::HeightAt(X, Y) + 80.f));
	if (ANAWolf* Wolf = GetWorld()->SpawnActorDeferred<ANAWolf>(ANAWolf::StaticClass(), Where, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
	{
		Wolf->SetDenLocation(WolfDenLocation);
		Wolf->FinishSpawning(Where);
	}
}

void ANAGameMode::SpawnDeer()
{
	const FVector2D Offset = FMath::RandPointInCircle(900.f);
	const float X = DeerHerdCenter.X + Offset.X;
	const float Y = DeerHerdCenter.Y + Offset.Y;
	const FTransform Where(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), FVector(X, Y, NATerrain::HeightAt(X, Y) + 90.f));
	if (ANADeer* Deer = GetWorld()->SpawnActorDeferred<ANADeer>(ANADeer::StaticClass(), Where, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
	{
		Deer->SetHerdCenter(DeerHerdCenter);
		Deer->FinishSpawning(Where);
	}
}

void ANAGameMode::OnBossKilled(ANABoss* Boss, AController* Killer)
{
	if (!Boss)
	{
		return;
	}
	if (ANAPlayerState* KillerState = Killer ? Killer->GetPlayerState<ANAPlayerState>() : nullptr)
	{
		KillerState->AddKill();
	}
	if (ANAGameState* State = GetGameState<ANAGameState>())
	{
		State->BroadcastKill(DisplayNameOf(nullptr, Killer), ANABoss::BossName());
	}
	BroadcastSystemMessage(FString::Printf(TEXT("%s has slain the ASHEN BEHEMOTH! Its heart lies in the Ash Crater."), *DisplayNameOf(nullptr, Killer)));
	// Everyone who fought nearby shares the reward.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Fighter = It->Get();
		const APawn* FighterPawn = Fighter ? Fighter->GetPawn() : nullptr;
		if (FighterPawn && FVector::Dist(FighterPawn->GetActorLocation(), Boss->GetActorLocation()) < 8000.f)
		{
			AwardXP(Fighter, 500, TEXT("Ashen Behemoth"));
		}
	}
	Boss->SetLifeSpan(120.f);
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &ANAGameMode::SpawnBoss, BossRespawnSeconds, false);
}

void ANAGameMode::SpawnShrines(const FTransform& ArenaTransform)
{
	UWorld* World = GetWorld();
	struct FShrineSpot { const TCHAR* Name; FVector Location; float Yaw; };
	const FVector2D Cabin = NATerrain::CabinCenter();
	const FVector2D Lake = NATerrain::LakeCenter();
	const FVector2D LakeShore = Lake + (FVector2D::ZeroVector - Lake).GetSafeNormal() * 9800.f; // Shore side facing the arena.
	const FShrineSpot Spots[] =
	{
		{ TEXT("Arena Shrine"),    ArenaTransform.TransformPosition(FVector(300.f, -2600.f, 0.f)), 0.f },
		{ TEXT("Cabin Shrine"),    FVector(Cabin.X + 200.f, Cabin.Y - 1300.f, NATerrain::HeightAt(Cabin.X + 200.f, Cabin.Y - 1300.f)), 180.f },
		{ TEXT("Lakeside Shrine"), FVector(LakeShore.X, LakeShore.Y, FMath::Max(NATerrain::WaterLevel() + 50.f, NATerrain::HeightAt(LakeShore.X, LakeShore.Y))), -45.f },
	};
	for (const FShrineSpot& Spot : Spots)
	{
		const FTransform Where(FRotator(0.f, Spot.Yaw, 0.f), Spot.Location);
		if (ANAAshShrine* Shrine = World->SpawnActorDeferred<ANAAshShrine>(ANAAshShrine::StaticClass(), Where,
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Shrine->SetShrineName(Spot.Name);
			Shrine->FinishSpawning(Where);
		}
	}
}

float ANAGameMode::ServerNow() const
{
	const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return State ? static_cast<float>(State->GetServerWorldTimeSeconds()) : 0.f;
}

bool ANAGameMode::HandlePlayerDeathItems(ANACharacterBase* Victim, APlayerController* PlayerController)
{
	ANAPlayerCharacter* PlayerBody = Cast<ANAPlayerCharacter>(Victim);
	ANAPlayerState* VictimState = PlayerController ? PlayerController->GetPlayerState<ANAPlayerState>() : nullptr;
	UNAInventoryComponent* Inventory = PlayerBody ? PlayerBody->GetInventoryComponent() : nullptr;
	if (!VictimState || !Inventory)
	{
		return false;
	}

	const float Now = ServerNow();
	ANAAshShrine* Shrine = Cast<ANAAshShrine>(VictimState->GetAttunedShrine());
	const float CooldownLeft = ANAPlayerState::RebirthCooldownSeconds - (Now - VictimState->LastRebirthTime);
	const bool bRebirth = Shrine && VictimState->GetAshCharges() > 0 && CooldownLeft <= 0.f;
	const TArray<FNAItemStack> Carried = Inventory->GetItems();
	ANAPlayerController* NAController = Cast<ANAPlayerController>(PlayerController);

	if (bRebirth)
	{
		FPendingRebirth& Pending = PendingRebirths.FindOrAdd(PlayerController);
		Pending.Shrine = Shrine;
		Pending.Items = Carried;
		VictimState->SetAshCharges(VictimState->GetAshCharges() - 1);
		VictimState->LastRebirthTime = Now;
		SystemMessage(NAController, FString::Printf(TEXT("ASH REBIRTH: you will rise at %s with your items. Charges left: %d"),
			*Shrine->GetShrineName(), VictimState->GetAshCharges()));
	}
	else
	{
		// Everything you carried falls where you died, for anyone to take.
		const FVector Center = Victim->GetActorLocation();
		int32 Index = 0;
		for (const FNAItemStack& Stack : Carried)
		{
			const float Angle = Index++ * 0.9f;
			const FVector Offset(FMath::Cos(Angle) * 90.f, FMath::Sin(Angle) * 90.f, 0.f);
			ANAItemPickup::Drop(GetWorld(), Center + Offset, Stack.ItemId, Stack.Count);
		}
		if (!Shrine)
		{
			SystemMessage(NAController, TEXT("You dropped your items. Attune to an Ash Shrine to be reborn with them next time."));
		}
		else if (VictimState->GetAshCharges() <= 0)
		{
			SystemMessage(NAController, TEXT("No Ash charges left - your items were dropped. Offer Ember Fangs at a shrine for more."));
		}
		else
		{
			SystemMessage(NAController, FString::Printf(TEXT("The shrine is still recovering (%d s) - your items were dropped."), FMath::CeilToInt(CooldownLeft)));
		}
	}
	for (const FNAItemStack& Stack : Carried)
	{
		Inventory->RemoveItem(Stack.ItemId, Stack.Count);
	}
	return bRebirth;
}

void ANAGameMode::SpawnCreature()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FTransform Where(FRotator(0.f, -45.f, 0.f), CreatureDenLocation);
	if (ANACreature* Creature = World->SpawnActorDeferred<ANACreature>(ANACreature::StaticClass(), Where,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
	{
		Creature->SetDenLocation(CreatureDenLocation);
		Creature->FinishSpawning(Where);
	}
}

void ANAGameMode::OnCreatureKilled(ANACreature* Creature, AController* Killer)
{
	if (!Creature)
	{
		return;
	}
	if (ANAPlayerState* KillerState = Killer ? Killer->GetPlayerState<ANAPlayerState>() : nullptr)
	{
		KillerState->AddKill();
	}
	if (ANAGameState* State = GetGameState<ANAGameState>())
	{
		State->BroadcastKill(DisplayNameOf(nullptr, Killer), Creature->GetSpeciesName());
	}
	AwardXP(Killer, Creature->GetXPReward(), Creature->GetSpeciesName());
	Creature->SetLifeSpan(CorpseLifeSeconds * 2.f);
	FTimerHandle Handle;
	switch (Creature->GetSpawnGroup())
	{
	case 0:  GetWorldTimerManager().SetTimer(Handle, this, &ANAGameMode::SpawnCreature, CreatureRespawnSeconds, false); break;
	case 1:  GetWorldTimerManager().SetTimer(Handle, this, &ANAGameMode::SpawnWolf, CreatureRespawnSeconds * 2.f, false); break;
	default: OnEventCreatureKilled(Creature, Killer); break; // World-event creature: no respawn.
	}
}

void ANAGameMode::SpawnEnemy(int32 PostIndex)
{
	UWorld* World = GetWorld();
	if (!World || !EnemyPosts.IsValidIndex(PostIndex))
	{
		return;
	}

	const FEnemyPost& Post = EnemyPosts[PostIndex];
	ANAEnemySoldier* Soldier = World->SpawnActorDeferred<ANAEnemySoldier>(ANAEnemySoldier::StaticClass(), Post.SpawnTransform,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Soldier)
	{
		Soldier->PatrolPoints = Post.Patrol;
		Soldier->Difficulty = EnemyDifficulty;
		Soldier->FinishSpawning(Post.SpawnTransform);
		EnemyPostByActor.Add(Soldier, PostIndex);
	}
}

void ANAGameMode::SetEnemyDifficulty(ENADifficulty NewDifficulty)
{
	EnemyDifficulty = NewDifficulty;
	for (TActorIterator<ANAEnemySoldier> It(GetWorld()); It; ++It)
	{
		if (ANAEnemyAIController* Brain = Cast<ANAEnemyAIController>(It->GetController()))
		{
			Brain->SetDifficulty(NewDifficulty);
		}
	}
	UE_LOG(LogNullAction, Log, TEXT("Enemy difficulty set to %s"), NAAI::DifficultyName(NewDifficulty));
}

void ANAGameMode::OnCharacterKilled(ANACharacterBase* Victim, AController* Killer)
{
	if (!Victim)
	{
		return;
	}

	// Score and kill feed (dummies don't count).
	if (!Victim->IsA<ANATrainingDummy>())
	{
		AController* VictimOwner = Victim->GetController();
		ANAPlayerState* KillerState = Killer ? Killer->GetPlayerState<ANAPlayerState>() : nullptr;
		if (KillerState && Killer != VictimOwner)
		{
			KillerState->AddKill();
		}
		if (ANAPlayerState* VictimState = VictimOwner ? VictimOwner->GetPlayerState<ANAPlayerState>() : nullptr)
		{
			VictimState->AddDeath();
		}
		if (ANAGameState* State = GetGameState<ANAGameState>())
		{
			State->BroadcastKill(DisplayNameOf(nullptr, Killer), DisplayNameOf(Victim, VictimOwner));
		}
	}

	// Enemy soldiers: a replacement arrives at the same post after a while.
	if (ANAEnemySoldier* Soldier = Cast<ANAEnemySoldier>(Victim))
	{
		AwardXP(Killer, 25, TEXT("Soldier"));
		Victim->SetLifeSpan(CorpseLifeSeconds);
		if (const int32* PostIndex = EnemyPostByActor.Find(Soldier))
		{
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateUObject(this, &ANAGameMode::SpawnEnemy, *PostIndex), EnemyRespawnSeconds, false);
			EnemyPostByActor.Remove(Soldier);
		}
		return;
	}

	AController* VictimController = Victim->GetController();
	if (!VictimController || !VictimController->IsPlayerController())
	{
		return; // Dummies manage themselves.
	}

	APlayerController* PlayerController = Cast<APlayerController>(VictimController);
	const bool bRebirth = HandlePlayerDeathItems(Victim, PlayerController);
	PlayerController->UnPossess();
	// Keep looking at your body while you wait.
	PlayerController->SetViewTarget(Victim);
	Victim->SetLifeSpan(CorpseLifeSeconds);

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle,
		FTimerDelegate::CreateUObject(this, &ANAGameMode::RespawnPlayer, TWeakObjectPtr<AController>(VictimController)),
		bRebirth ? 3.f : RespawnDelaySeconds, false);
}

void ANAGameMode::RespawnPlayer(TWeakObjectPtr<AController> Controller)
{
	if (!Controller.IsValid() || Controller->GetPawn())
	{
		return;
	}
	RestartPlayer(Controller.Get());

	FPendingRebirth Pending;
	if (!PendingRebirths.RemoveAndCopyValue(Controller, Pending))
	{
		return;
	}
	ANAPlayerCharacter* Reborn = Cast<ANAPlayerCharacter>(Controller->GetPawn());
	ANAAshShrine* Shrine = Pending.Shrine.Get();
	if (!Reborn || !Shrine)
	{
		return;
	}
	// Rise at the shrine: your items return, but you come back weakened and Ash-touched (slower) for a while.
	const FTransform At = Shrine->GetRebirthTransform();
	Reborn->TeleportTo(At.GetLocation(), At.Rotator(), false, true);
	Controller->ClientSetRotation(At.Rotator());
	if (UNAInventoryComponent* Inventory = Reborn->GetInventoryComponent())
	{
		for (const FNAItemStack& Stack : Pending.Items)
		{
			Inventory->AddItem(Stack.ItemId, Stack.Count, false);
		}
	}
	if (UNAHealthComponent* Health = Reborn->GetHealthComponent())
	{
		Health->Revive(0.6f);
	}
	if (ANAPlayerState* State = Controller->GetPlayerState<ANAPlayerState>())
	{
		State->SetAshTouchedUntil(ServerNow() + 120.f);
	}
	Shrine->MulticastRebirthFlare();
}
