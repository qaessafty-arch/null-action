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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ----- Current world event (server sets; everyone's HUD shows it) -----
	bool IsEventActive() const { return bEventActive; }
	const FString& GetEventTitle() const { return EventTitle; }
	const FString& GetEventDetail() const { return EventDetail; }
	FVector GetEventLocation() const { return EventLocation; }
	float GetEventEndTime() const { return EventEndTime; }
	/** Server only. */
	void SetEvent(const FString& Title, const FString& Detail, const FVector& Location, float EndServerTime)
	{
		bEventActive = true; EventTitle = Title; EventDetail = Detail; EventLocation = Location; EventEndTime = EndServerTime;
	}
	void SetEventDetail(const FString& Detail) { EventDetail = Detail; }
	void SetEventLocation(const FVector& Location) { EventLocation = Location; }
	void ClearEvent() { bEventActive = false; EventTitle.Reset(); EventDetail.Reset(); }

protected:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastKill(const FString& Killer, const FString& Victim);

	UPROPERTY(Replicated) bool bEventActive = false;
	UPROPERTY(Replicated) FString EventTitle;
	UPROPERTY(Replicated) FString EventDetail;
	UPROPERTY(Replicated) FVector_NetQuantize EventLocation;
	UPROPERTY(Replicated) float EventEndTime = 0.f;

private:
	TArray<FNAKillFeedEntry> KillFeed;
};
