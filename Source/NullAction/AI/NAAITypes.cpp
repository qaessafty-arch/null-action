#include "AI/NAAITypes.h"

FNADifficultyProfile NAAI::GetDifficultyProfile(ENADifficulty Difficulty)
{
	FNADifficultyProfile P;
	switch (Difficulty)
	{
	case ENADifficulty::Easy:
		P.ReactionSeconds = 1.6f;  P.SightRadius = 2500.f; P.HearingRange = 2000.f;
		P.AimErrorDegrees = 6.f;   P.HeadshotChance = 0.f;  P.BurstSeconds = 0.25f; P.BurstPauseSeconds = 1.2f;
		P.CoverChance = 0.15f;     P.RetreatHealthFraction = 0.f; P.SearchSeconds = 6.f; P.PreferredRangeCm = 1200.f;
		P.bInvestigatesSuspicion = false; P.bAlertsAllies = false; P.bFlanks = false;
		break;
	case ENADifficulty::Normal:
		// Defaults above.
		break;
	case ENADifficulty::Hard:
		P.ReactionSeconds = 0.6f;  P.SightRadius = 4500.f; P.HearingRange = 3800.f;
		P.AimErrorDegrees = 2.f;   P.HeadshotChance = 0.25f; P.BurstSeconds = 0.4f; P.BurstPauseSeconds = 0.55f;
		P.CoverChance = 0.8f;      P.RetreatHealthFraction = 0.35f; P.SearchSeconds = 15.f; P.PreferredRangeCm = 1800.f;
		P.bInvestigatesSuspicion = true; P.bAlertsAllies = true; P.bFlanks = false;
		break;
	case ENADifficulty::Expert:
		P.ReactionSeconds = 0.35f; P.SightRadius = 5500.f; P.HearingRange = 4500.f;
		P.AimErrorDegrees = 1.f;   P.HeadshotChance = 0.45f; P.BurstSeconds = 0.5f; P.BurstPauseSeconds = 0.35f;
		P.CoverChance = 0.95f;     P.RetreatHealthFraction = 0.4f; P.SearchSeconds = 20.f; P.PreferredRangeCm = 2000.f;
		P.bInvestigatesSuspicion = true; P.bAlertsAllies = true; P.bFlanks = true;
		break;
	}
	return P;
}

const TCHAR* NAAI::DifficultyName(ENADifficulty Difficulty)
{
	switch (Difficulty)
	{
	case ENADifficulty::Easy:   return TEXT("Easy");
	case ENADifficulty::Normal: return TEXT("Normal");
	case ENADifficulty::Hard:   return TEXT("Hard");
	case ENADifficulty::Expert: return TEXT("Expert");
	}
	return TEXT("?");
}

bool NAAI::ParseDifficulty(const FString& Text, ENADifficulty& OutDifficulty)
{
	const ENADifficulty All[] = { ENADifficulty::Easy, ENADifficulty::Normal, ENADifficulty::Hard, ENADifficulty::Expert };
	for (const ENADifficulty Candidate : All)
	{
		if (Text.Equals(DifficultyName(Candidate), ESearchCase::IgnoreCase))
		{
			OutDifficulty = Candidate;
			return true;
		}
	}
	return false;
}
