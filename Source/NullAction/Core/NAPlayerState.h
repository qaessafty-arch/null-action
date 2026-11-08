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

/** Playable class; each has one power on Q. */
UENUM(BlueprintType)
enum class ENAArchetype : uint8
{
	None,
	Phantom,
	Hunter,
	Storm,
	Guardian
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

	// ----- Ash Rebirth -----
	int32 GetAshCharges() const { return AshCharges; }
	const FString& GetAttunedShrineName() const { return AttunedShrineName; }
	float GetAshTouchedUntil() const { return AshTouchedUntil; }
	AActor* GetAttunedShrine() const { return AttunedShrine.Get(); }
	/** Server only. */
	void SetAshCharges(int32 NewCharges) { AshCharges = FMath::Clamp(NewCharges, 0, MaxAshCharges); }
	void AttuneTo(AActor* Shrine, const FString& ShrineName) { AttunedShrine = Shrine; AttunedShrineName = ShrineName; }
	void SetAshTouchedUntil(float ServerTime) { AshTouchedUntil = ServerTime; }
	/** Server world time of the last rebirth (replicated so the death screen can show the cooldown). */
	UPROPERTY(Replicated) float LastRebirthTime = -10000.f;
	static constexpr int32 MaxAshCharges = 3;
	static constexpr float RebirthCooldownSeconds = 300.f;

	// ----- Class -----
	ENAArchetype GetArchetype() const { return Archetype; }
	void SetArchetype(ENAArchetype NewArchetype) { Archetype = NewArchetype; }

protected:
	UPROPERTY(Replicated) int32 Kills = 0;
	UPROPERTY(Replicated) int32 Deaths = 0;
	UPROPERTY(Replicated) ENAPlayerRole PlayerRole = ENAPlayerRole::Player;
	UPROPERTY(Replicated) int32 AshCharges = 1;
	UPROPERTY(Replicated) FString AttunedShrineName;
	UPROPERTY(Replicated) float AshTouchedUntil = -1.f;
	UPROPERTY(Replicated) ENAArchetype Archetype = ENAArchetype::Hunter;
	TWeakObjectPtr<AActor> AttunedShrine;
};
