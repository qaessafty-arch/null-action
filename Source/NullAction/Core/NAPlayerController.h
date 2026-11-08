#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NAPlayerController.generated.h"

/**
 * NULL-ACTION player controller. Hosts development console commands.
 * Commands that change the match only work for whoever owns the server (standalone or listen-server
 * host); a connected client is refused. Owner/admin permissions will replace this check later.
 */
UCLASS()
class NULLACTION_API ANAPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Console: "NADifficulty Easy|Normal|Hard|Expert". Host only. */
	UFUNCTION(Exec)
	void NADifficulty(const FString& Level);
};
