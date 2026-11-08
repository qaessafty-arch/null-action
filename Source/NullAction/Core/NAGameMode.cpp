#include "Core/NAGameMode.h"

#include "NullAction.h"
#include "Character/NAPlayerCharacter.h"
#include "Core/NAHUD.h"
#include "EngineUtils.h"
#include "AI/NAEnemyAIController.h"
#include "AI/NAEnemySoldier.h"
#include "Combat/NATrainingDummy.h"
#include "Core/NAPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Interaction/NAPickup.h"
#include "TimerManager.h"
#include "Weapons/NAWeaponTypes.h"
#include "World/NATestArena.h"

ANAGameMode::ANAGameMode()
{
	DefaultPawnClass = ANAPlayerCharacter::StaticClass();
	HUDClass = ANAHUD::StaticClass();
	PlayerControllerClass = ANAPlayerController::StaticClass();
	TestArenaClass = ANATestArena::StaticClass();
}

void ANAGameMode::StartPlay()
{
	EnsureTestArenaIfLevelIsEmpty();
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

	// Enemy soldiers: a replacement arrives at the same post after a while.
	if (ANAEnemySoldier* Soldier = Cast<ANAEnemySoldier>(Victim))
	{
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
	PlayerController->UnPossess();
	// Keep looking at your body while you wait.
	PlayerController->SetViewTarget(Victim);
	Victim->SetLifeSpan(CorpseLifeSeconds);

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle,
		FTimerDelegate::CreateUObject(this, &ANAGameMode::RespawnPlayer, TWeakObjectPtr<AController>(VictimController)),
		RespawnDelaySeconds, false);
}

void ANAGameMode::RespawnPlayer(TWeakObjectPtr<AController> Controller)
{
	if (Controller.IsValid() && !Controller->GetPawn())
	{
		RestartPlayer(Controller.Get());
	}
}
