#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NAPlayerRecord.generated.h"

/**
 * Server-side saved record for one player, keyed by their stable ID. Lives on the server machine
 * only (Saved/SaveGames); clients never write it. Version lets future changes migrate old saves.
 */
UCLASS()
class NULLACTION_API UNAPlayerRecord : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Version = 1;
	UPROPERTY() FString LastKnownName;
	UPROPERTY() int32 Kills = 0;
	UPROPERTY() int32 Deaths = 0;
	UPROPERTY() float PlaySeconds = 0.f;
	UPROPERTY() FDateTime FirstSeen;
	UPROPERTY() FDateTime LastSeen;
};
