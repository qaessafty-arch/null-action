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
	Guardian,
	Engineer
};

/** Progression skills: 3 ranks each, bought with skill points earned by levelling up. */
UENUM(BlueprintType)
enum class ENASkill : uint8
{
	Vitality,     // +10 max health per rank
	Endurance,    // +12% stamina recovery, less sprint drain per rank
	HuntersEye,   // +8% damage to creatures and bosses per rank
	Survivalist,  // -12% hunger and thirst drain per rank
	PowerMastery, // -10% power cooldown, +10% energy recovery per rank
	Count UMETA(Hidden)
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

	// ----- Progression (server changes, everyone sees) -----
	static constexpr int32 SkillCount = static_cast<int32>(ENASkill::Count);
	static constexpr int32 MaxSkillRank = 3;
	static constexpr int32 MaxLevel = 30;
	int32 GetPlayerLevel() const { return PlayerLevel; }
	int32 GetXP() const { return XP; }
	int32 GetSkillPoints() const { return SkillPoints; }
	int32 GetSkillRank(ENASkill Skill) const { return SkillRanks.IsValidIndex(static_cast<int32>(Skill)) ? SkillRanks[static_cast<int32>(Skill)] : 0; }
	static int32 XPForNextLevel(int32 FromLevel) { return 100 + 60 * (FromLevel - 1); }
	static FString SkillName(ENASkill Skill);
	static FString SkillDescription(ENASkill Skill);
	/** Server: returns true if this caused a level-up. */
	bool GrantXP(int32 Amount);
	/** Server: spend a point. */
	bool TryBuySkill(ENASkill Skill);
	/** Server: restore from a saved record. */
	void SetProgress(int32 NewLevel, int32 NewXP, int32 NewPoints, const TArray<int32>& NewRanks);
	const TArray<int32>& GetSkillRanks() const { return SkillRanks; }

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
	UPROPERTY(Replicated) int32 PlayerLevel = 1;
	UPROPERTY(Replicated) int32 XP = 0;
	UPROPERTY(Replicated) int32 SkillPoints = 0;
	UPROPERTY(Replicated) TArray<int32> SkillRanks;
	TWeakObjectPtr<AActor> AttunedShrine;
};
