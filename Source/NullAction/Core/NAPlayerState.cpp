#include "Core/NAPlayerState.h"

#include "Net/UnrealNetwork.h"

void ANAPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAPlayerState, Kills);
	DOREPLIFETIME(ANAPlayerState, Deaths);
	DOREPLIFETIME(ANAPlayerState, PlayerRole);
}

void ANAPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	// Used by the engine when a disconnected player comes back: keep their score and role.
	if (ANAPlayerState* Target = Cast<ANAPlayerState>(PlayerState))
	{
		Target->Kills = Kills;
		Target->Deaths = Deaths;
		Target->PlayerRole = PlayerRole;
	}
}

FString ANAPlayerState::GetStableId() const
{
	const FUniqueNetIdRepl& Id = GetUniqueId();
	if (Id.IsValid())
	{
		return Id.ToString();
	}
	// No online identity (e.g. offline editor test): fall back to the player name. Not secure; see Docs/MULTIPLAYER.md.
	return FString::Printf(TEXT("name:%s"), *GetPlayerName());
}
