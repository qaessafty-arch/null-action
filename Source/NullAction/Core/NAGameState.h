#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "NAGameState.generated.h"

USTRUCT()
struct FNAKillFeedEntry
{
	GENERATED_BODY()

	UPROPERTY() FString Killer;
	UPROPERTY() FString Victim;
	UPROPERTY() bool bHeadshotOrSpecial = false;
	float LocalTime = 0.f;
};

/** Match-wide state every client sees. Kill feed events are pushed by the server. */
UCLASS()
class NULLACTION_API ANAGameState : public AGameState
{
	GENERATED_BODY()

public:
	/** Server only. */
	void BroadcastKill(const FString& Killer, const FString& Victim);

	const TArray<FNAKillFeedEntry>& GetKillFeed() const { return KillFeed; }

protected:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastKill(const FString& Killer, const FString& Victim);

private:
	TArray<FNAKillFeedEntry> KillFeed;
};
