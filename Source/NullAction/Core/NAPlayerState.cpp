#include "Core/NAPlayerState.h"

#include "Net/UnrealNetwork.h"

void ANAPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAPlayerState, Kills);
	DOREPLIFETIME(ANAPlayerState, Deaths);
	DOREPLIFETIME(ANAPlayerState, PlayerRole);
	DOREPLIFETIME(ANAPlayerState, AshCharges);
	DOREPLIFETIME(ANAPlayerState, AttunedShrineName);
	DOREPLIFETIME(ANAPlayerState, AshTouchedUntil);
	DOREPLIFETIME(ANAPlayerState, Archetype);
	DOREPLIFETIME(ANAPlayerState, LastRebirthTime);
	DOREPLIFETIME(ANAPlayerState, PlayerLevel);
	DOREPLIFETIME(ANAPlayerState, XP);
	DOREPLIFETIME(ANAPlayerState, SkillPoints);
	DOREPLIFETIME(ANAPlayerState, SkillRanks);
}

FString ANAPlayerState::SkillName(ENASkill Skill)
{
	switch (Skill)
	{
	case ENASkill::Vitality:     return TEXT("Vitality");
	case ENASkill::Endurance:    return TEXT("Endurance");
	case ENASkill::HuntersEye:   return TEXT("Hunter's Eye");
	case ENASkill::Survivalist:  return TEXT("Survivalist");
	case ENASkill::PowerMastery: return TEXT("Power Mastery");
	default:                     return TEXT("?");
	}
}

FString ANAPlayerState::SkillDescription(ENASkill Skill)
{
	switch (Skill)
	{
	case ENASkill::Vitality:     return TEXT("+10 max health per rank");
	case ENASkill::Endurance:    return TEXT("+12% stamina recovery, less sprint drain");
	case ENASkill::HuntersEye:   return TEXT("+8% damage to creatures and bosses");
	case ENASkill::Survivalist:  return TEXT("-12% hunger and thirst drain");
	case ENASkill::PowerMastery: return TEXT("-10% power cooldown, +10% energy recovery");
	default:                     return FString();
	}
}

bool ANAPlayerState::GrantXP(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0 || PlayerLevel >= MaxLevel)
	{
		return false;
	}
	XP += Amount;
	bool bLevelled = false;
	while (PlayerLevel < MaxLevel && XP >= XPForNextLevel(PlayerLevel))
	{
		XP -= XPForNextLevel(PlayerLevel);
		++PlayerLevel;
		++SkillPoints;
		bLevelled = true;
	}
	if (PlayerLevel >= MaxLevel)
	{
		XP = 0;
	}
	return bLevelled;
}

bool ANAPlayerState::TryBuySkill(ENASkill Skill)
{
	const int32 Index = static_cast<int32>(Skill);
	if (!HasAuthority() || SkillPoints <= 0 || Index < 0 || Index >= SkillCount)
	{
		return false;
	}
	if (SkillRanks.Num() < SkillCount)
	{
		SkillRanks.SetNumZeroed(SkillCount);
	}
	if (SkillRanks[Index] >= MaxSkillRank)
	{
		return false;
	}
	++SkillRanks[Index];
	--SkillPoints;
	return true;
}

void ANAPlayerState::SetProgress(int32 NewLevel, int32 NewXP, int32 NewPoints, const TArray<int32>& NewRanks)
{
	PlayerLevel = FMath::Clamp(NewLevel, 1, MaxLevel);
	XP = FMath::Max(0, NewXP);
	SkillPoints = FMath::Clamp(NewPoints, 0, MaxLevel);
	SkillRanks = NewRanks;
	SkillRanks.SetNumZeroed(SkillCount);
	for (int32& Rank : SkillRanks)
	{
		Rank = FMath::Clamp(Rank, 0, MaxSkillRank); // Never trust a saved file blindly.
	}
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
		Target->AshCharges = AshCharges;
		Target->AttunedShrineName = AttunedShrineName;
		Target->AttunedShrine = AttunedShrine;
		Target->Archetype = Archetype;
		Target->LastRebirthTime = LastRebirthTime;
		Target->PlayerLevel = PlayerLevel;
		Target->XP = XP;
		Target->SkillPoints = SkillPoints;
		Target->SkillRanks = SkillRanks;
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
