#include "Core/NAPlayerController.h"

#include "AI/NAAITypes.h"
#include "Core/NAGameMode.h"
#include "Engine/World.h"

void ANAPlayerController::NADifficulty(const FString& Level)
{
	ANAGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANAGameMode>() : nullptr;
	if (!GameMode || !IsLocalController())
	{
		ClientMessage(TEXT("Only the host can change difficulty."));
		return;
	}

	ENADifficulty NewDifficulty;
	if (!NAAI::ParseDifficulty(Level, NewDifficulty))
	{
		ClientMessage(TEXT("Usage: NADifficulty Easy | Normal | Hard | Expert"));
		return;
	}

	GameMode->SetEnemyDifficulty(NewDifficulty);
	ClientMessage(FString::Printf(TEXT("Enemy difficulty: %s"), NAAI::DifficultyName(NewDifficulty)));
}
