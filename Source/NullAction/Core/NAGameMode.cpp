#include "Core/NAGameMode.h"

#include "NullAction.h"
#include "Character/NAPlayerCharacter.h"
#include "Core/NAHUD.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "World/NATestArena.h"

ANAGameMode::ANAGameMode()
{
	DefaultPawnClass = ANAPlayerCharacter::StaticClass();
	HUDClass = ANAHUD::StaticClass();
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

	if (TActorIterator<APlayerStart> ExistingStart(World); ExistingStart)
	{
		return; // A real level with its own spawn points: leave it alone.
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
		World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), TestArena->GetSpawnTransform(Index), Params);
	}

	UE_LOG(LogNullAction, Log, TEXT("Level has no PlayerStarts: built test arena with %d spawn points."), TestArenaPlayerStarts);
}
