#include "Core/NAGameState.h"
#include "Net/UnrealNetwork.h"

#include "Engine/World.h"

void ANAGameState::BroadcastKill(const FString& Killer, const FString& Victim)
{
	if (HasAuthority())
	{
		MulticastKill(Killer, Victim);
	}
}

void ANAGameState::MulticastKill_Implementation(const FString& Killer, const FString& Victim)
{
	FNAKillFeedEntry Entry;
	Entry.Killer = Killer;
	Entry.Victim = Victim;
	Entry.LocalTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	KillFeed.Add(Entry);
	if (KillFeed.Num() > 6)
	{
		KillFeed.RemoveAt(0);
	}
}

void ANAGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAGameState, bEventActive);
	DOREPLIFETIME(ANAGameState, EventTitle);
	DOREPLIFETIME(ANAGameState, EventDetail);
	DOREPLIFETIME(ANAGameState, EventLocation);
	DOREPLIFETIME(ANAGameState, EventEndTime);
}
