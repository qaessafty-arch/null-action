#include "Core/NAPlayerState.h"

#include "Net/UnrealNetwork.h"
#include "Character/NACharacterBase.h"

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
	DOREPLIFETIME(ANAPlayerState, Appearance);
}

namespace NAAppearance
{
	static const FLinearColor Outfits[OutfitCount] = { FLinearColor(0.16f, 0.2f, 0.12f), FLinearColor(0.08f, 0.08f, 0.09f), FLinearColor(0.3f, 0.26f, 0.18f),
		FLinearColor(0.12f, 0.16f, 0.24f), FLinearColor(0.35f, 0.1f, 0.06f), FLinearColor(0.45f, 0.45f, 0.43f) };
	static const FLinearColor Pants[OutfitCount] = { FLinearColor(0.09f, 0.09f, 0.1f), FLinearColor(0.05f, 0.05f, 0.05f), FLinearColor(0.18f, 0.15f, 0.1f),
		FLinearColor(0.07f, 0.08f, 0.12f), FLinearColor(0.1f, 0.08f, 0.07f), FLinearColor(0.2f, 0.2f, 0.2f) };
	static const TCHAR* OutfitNames[OutfitCount] = { TEXT("Ranger (olive)"), TEXT("Shadow (black)"), TEXT("Nomad (desert)"), TEXT("Tidewalker (navy)"), TEXT("Ember (rust)"), TEXT("Ashen (grey)") };
	static const FLinearColor Skins[SkinCount] = { FLinearColor(0.85f, 0.66f, 0.55f), FLinearColor(0.55f, 0.38f, 0.28f), FLinearColor(0.4f, 0.26f, 0.17f),
		FLinearColor(0.26f, 0.16f, 0.1f), FLinearColor(0.72f, 0.56f, 0.4f) };
	static const TCHAR* SkinNames[SkinCount] = { TEXT("Light"), TEXT("Medium"), TEXT("Tan"), TEXT("Deep"), TEXT("Olive") };
	static const FLinearColor Accents[AccentCount] = { FLinearColor(0.1f, 0.75f, 0.95f), FLinearColor(0.95f, 0.22f, 0.16f), FLinearColor(0.3f, 1.f, 0.4f),
		FLinearColor(1.f, 0.65f, 0.1f), FLinearColor(0.75f, 0.4f, 1.f), FLinearColor(0.95f, 0.95f, 0.95f) };
	static const TCHAR* AccentNames[AccentCount] = { TEXT("Ion blue"), TEXT("Null red"), TEXT("Signal green"), TEXT("Ember gold"), TEXT("Void violet"), TEXT("White") };

	FLinearColor OutfitColor(int32 Index) { return Outfits[FMath::Clamp(Index, 0, OutfitCount - 1)]; }
	FLinearColor PantsColor(int32 Index)  { return Pants[FMath::Clamp(Index, 0, OutfitCount - 1)]; }
	FLinearColor SkinColor(int32 Index)   { return Skins[FMath::Clamp(Index, 0, SkinCount - 1)]; }
	FLinearColor AccentColor(int32 Index) { return Accents[FMath::Clamp(Index, 0, AccentCount - 1)]; }
	const TCHAR* OutfitName(int32 Index)  { return OutfitNames[FMath::Clamp(Index, 0, OutfitCount - 1)]; }
	const TCHAR* SkinName(int32 Index)    { return SkinNames[FMath::Clamp(Index, 0, SkinCount - 1)]; }
	const TCHAR* AccentName(int32 Index)  { return AccentNames[FMath::Clamp(Index, 0, AccentCount - 1)]; }
}

void ANAPlayerState::SetAppearance(const FNAAppearance& NewAppearance)
{
	Appearance = NewAppearance;
	Appearance.Outfit = static_cast<uint8>(FMath::Clamp<int32>(NewAppearance.Outfit, 0, NAAppearance::OutfitCount - 1));
	Appearance.Skin = static_cast<uint8>(FMath::Clamp<int32>(NewAppearance.Skin, 0, NAAppearance::SkinCount - 1));
	Appearance.Accent = static_cast<uint8>(FMath::Clamp<int32>(NewAppearance.Accent, 0, NAAppearance::AccentCount - 1));
	OnRep_Appearance();
}

void ANAPlayerState::OnRep_Appearance()
{
	if (ANACharacterBase* Body = GetPawn<ANACharacterBase>())
	{
		Body->ApplyAppearance(Appearance);
	}
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
		Target->Appearance = Appearance;
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
