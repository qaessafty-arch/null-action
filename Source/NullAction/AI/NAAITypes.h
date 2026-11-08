#pragma once

#include "CoreMinimal.h"
#include "NAAITypes.generated.h"

UENUM(BlueprintType)
enum class ENADifficulty : uint8
{
	Easy,
	Normal,
	Hard,
	Expert
};

UENUM(BlueprintType)
enum class ENAAlertState : uint8
{
	Unaware,
	Suspicious,
	Combat
};

/**
 * How an enemy thinks at a given difficulty. Higher difficulties make better decisions
 * (faster reactions, cover use, flanking, retreating, calling allies), not just more health.
 */
struct FNADifficultyProfile
{
	float ReactionSeconds = 1.f;        // Time to fully notice a visible player at mid range.
	float SightRadius = 3500.f;
	float HearingRange = 3000.f;
	float AimErrorDegrees = 3.5f;
	float HeadshotChance = 0.1f;        // Chance a burst aims at the head instead of the chest.
	float BurstSeconds = 0.35f;
	float BurstPauseSeconds = 0.8f;
	float CoverChance = 0.6f;           // Chance to take cover when reloading / repositioning.
	float RetreatHealthFraction = 0.f;  // Retreat once below this health (0 = never).
	float SearchSeconds = 10.f;
	float PreferredRangeCm = 1500.f;
	bool bInvestigatesSuspicion = true; // Walks over to check half-noticed movement.
	bool bAlertsAllies = false;         // Tells nearby soldiers where you are.
	bool bFlanks = false;               // Prefers cover to your side rather than straight ahead.
};

namespace NAAI
{
	NULLACTION_API FNADifficultyProfile GetDifficultyProfile(ENADifficulty Difficulty);
	NULLACTION_API const TCHAR* DifficultyName(ENADifficulty Difficulty);
	NULLACTION_API bool ParseDifficulty(const FString& Text, ENADifficulty& OutDifficulty);
}
