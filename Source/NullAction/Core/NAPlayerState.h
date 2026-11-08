#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NAPlayerState.generated.h"

/** Permission level, granted only by the server (see Docs/MULTIPLAYER.md). */
UENUM(BlueprintType)
enum class ENAPlayerRole : uint8
{
	Player,
	Moderator,
	Admin,
	Owner
};

/** Per-player data every client can see: name (engine), score, role. Survives a reconnect. */
UCLASS()
class NULLACTION_API ANAPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

	int32 GetKills() const { return Kills; }
	int32 GetDeaths() const { return Deaths; }
	ENAPlayerRole GetRole() const { return PlayerRole; }
	bool HasRole(ENAPlayerRole Minimum) const { return static_cast<uint8>(PlayerRole) >= static_cast<uint8>(Minimum); }

	/** Stable ID used for roles, bans and saved records. */
	FString GetStableId() const;

	/** Server only. */
	void AddKill() { ++Kills; }
	void AddDeath() { ++Deaths; }
	void SetStats(int32 NewKills, int32 NewDeaths) { Kills = NewKills; Deaths = NewDeaths; }
	void SetRole(ENAPlayerRole NewRole) { PlayerRole = NewRole; }

protected:
	UPROPERTY(Replicated) int32 Kills = 0;
	UPROPERTY(Replicated) int32 Deaths = 0;
	UPROPERTY(Replicated) ENAPlayerRole PlayerRole = ENAPlayerRole::Player;
};
