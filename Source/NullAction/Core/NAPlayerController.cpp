#include "Core/NAPlayerController.h"

#include "Core/NAGameMode.h"
#include "Core/NAPlayerState.h"
#include "Character/NACharacterBase.h"
#include "Engine/World.h"

// ----- Chat ----------------------------------------------------------------------------------------

void ANAPlayerController::NAChat(const FString& Message)   { ServerSendChat(ENAChatChannel::All, FString(), Message); }
void ANAPlayerController::NATeam(const FString& Message)   { ServerSendChat(ENAChatChannel::Team, FString(), Message); }
void ANAPlayerController::NALocal(const FString& Message)  { ServerSendChat(ENAChatChannel::Proximity, FString(), Message); }
void ANAPlayerController::NAWhisper(const FString& PlayerName, const FString& Message) { ServerSendChat(ENAChatChannel::Private, PlayerName, Message); }

void ANAPlayerController::NAMute(const FString& PlayerName)
{
	MutedNames.Add(PlayerName.ToLower());
	ClientReceiveChat(ENAChatChannel::System, FString(), FString::Printf(TEXT("Muted %s (only for you)."), *PlayerName));
}

void ANAPlayerController::NAUnmute(const FString& PlayerName)
{
	MutedNames.Remove(PlayerName.ToLower());
	ClientReceiveChat(ENAChatChannel::System, FString(), FString::Printf(TEXT("Unmuted %s."), *PlayerName));
}

void ANAPlayerController::NAReport(const FString& PlayerName, const FString& Reason)
{
	ServerReport(PlayerName, Reason);
}

void ANAPlayerController::NAClass(const FString& ClassName)
{
	ServerSetClass(ClassName);
}

void ANAPlayerController::ServerSetClass_Implementation(const FString& ClassName)
{
	ANAPlayerState* State = GetPlayerState<ANAPlayerState>();
	const ANACharacterBase* Body = Cast<ANACharacterBase>(GetPawn());
	if (!State || (Body && Body->IsDowned()))
	{
		return;
	}
	struct FClassName { const TCHAR* Name; ENAArchetype Value; const TCHAR* Power; };
	const FClassName Classes[] =
	{
		{ TEXT("Phantom"),  ENAArchetype::Phantom,  TEXT("Q: Cloak - invisible for 8 s; shooting or taking damage reveals you.") },
		{ TEXT("Hunter"),   ENAArchetype::Hunter,   TEXT("Q: Tracking Pulse - see enemies and creatures within 80 m through walls for 10 s.") },
		{ TEXT("Storm"),    ENAArchetype::Storm,    TEXT("Q: Lightning Strike - 70 damage where you aim, after a short warning.") },
		{ TEXT("Guardian"), ENAArchetype::Guardian, TEXT("Q: Restoration Field - heals you and teammates nearby; faster revives inside it.") },
		{ TEXT("Engineer"), ENAArchetype::Engineer, TEXT("Q: Energy Barrier - a shield wall in front of you that stops bullets for 10 s (300 health).") },
	};
	for (const FClassName& Entry : Classes)
	{
		if (ClassName.Equals(Entry.Name, ESearchCase::IgnoreCase))
		{
			State->SetArchetype(Entry.Value);
			ClientReceiveChat(ENAChatChannel::System, FString(), FString::Printf(TEXT("Class: %s. %s"), Entry.Name, Entry.Power));
			return;
		}
	}
	ClientReceiveChat(ENAChatChannel::System, FString(), TEXT("Usage: NAClass Phantom | Hunter | Storm | Guardian | Engineer"));
}

void ANAPlayerController::ServerSendChat_Implementation(ENAChatChannel Channel, const FString& Target, const FString& Message)
{
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->HandleChat(this, Channel, Target, Message);
	}
}

void ANAPlayerController::ServerReport_Implementation(const FString& PlayerName, const FString& Reason)
{
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->HandleReport(this, PlayerName, Reason);
	}
}

void ANAPlayerController::ClientReceiveChat_Implementation(ENAChatChannel Channel, const FString& Sender, const FString& Text)
{
	if (!Sender.IsEmpty() && MutedNames.Contains(Sender.ToLower()))
	{
		return;
	}
	FNAChatLine Line;
	Line.Channel = Channel;
	Line.Sender = Sender;
	Line.Text = Text;
	Line.LocalTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	ChatLog.Add(Line);
	if (ChatLog.Num() > 50)
	{
		ChatLog.RemoveAt(0);
	}
}

// ----- Admin ---------------------------------------------------------------------------------------

void ANAPlayerController::SendAdmin(ENAAdminCommand Command, const FString& Arg1, const FString& Arg2, const FVector& Location)
{
	ServerAdminCommand(Command, Arg1, Arg2, Location);
}

void ANAPlayerController::NAGod()                                   { SendAdmin(ENAAdminCommand::God); }
void ANAPlayerController::NAFly()                                   { SendAdmin(ENAAdminCommand::Fly); }
void ANAPlayerController::NAHeal()                                  { SendAdmin(ENAAdminCommand::Heal); }
void ANAPlayerController::NATeleport(float X, float Y, float Z)     { SendAdmin(ENAAdminCommand::Teleport, FString(), FString(), FVector(X, Y, Z)); }
void ANAPlayerController::NAGoto(const FString& PlayerName)         { SendAdmin(ENAAdminCommand::GoTo, PlayerName); }
void ANAPlayerController::NABring(const FString& PlayerName)        { SendAdmin(ENAAdminCommand::Bring, PlayerName); }
void ANAPlayerController::NAGive(const FString& What, const FString& Count) { SendAdmin(ENAAdminCommand::Give, What, Count); }
void ANAPlayerController::NASpawn(const FString& What)              { SendAdmin(ENAAdminCommand::Spawn, What); }
void ANAPlayerController::NADifficulty(const FString& Level)        { SendAdmin(ENAAdminCommand::Difficulty, Level); }
void ANAPlayerController::NAKick(const FString& PlayerName)         { SendAdmin(ENAAdminCommand::Kick, PlayerName); }
void ANAPlayerController::NABan(const FString& PlayerName)          { SendAdmin(ENAAdminCommand::Ban, PlayerName); }
void ANAPlayerController::NASetRole(const FString& PlayerName, const FString& RoleName) { SendAdmin(ENAAdminCommand::SetRole, PlayerName, RoleName); }
void ANAPlayerController::NAPlayers()                               { SendAdmin(ENAAdminCommand::ListPlayers); }
void ANAPlayerController::NAWeather(const FString& Weather)         { SendAdmin(ENAAdminCommand::Weather, Weather); }
void ANAPlayerController::NATime(float Hours)                       { SendAdmin(ENAAdminCommand::Time, FString::SanitizeFloat(Hours)); }
void ANAPlayerController::NADayLength(float Minutes)                { SendAdmin(ENAAdminCommand::DayLength, FString::SanitizeFloat(Minutes)); }
void ANAPlayerController::NASurvival(const FString& Level)          { SendAdmin(ENAAdminCommand::Survival, Level); }
void ANAPlayerController::NAEvent(const FString& Which)             { SendAdmin(ENAAdminCommand::Event, Which); }
void ANAPlayerController::NAGiveXP(int32 Amount)                    { SendAdmin(ENAAdminCommand::GiveXP, FString::FromInt(Amount)); }

void ANAPlayerController::NASkill(const FString& SkillName)
{
	for (int32 Index = 0; Index < ANAPlayerState::SkillCount; ++Index)
	{
		const FString Name = ANAPlayerState::SkillName(static_cast<ENASkill>(Index)).Replace(TEXT("'"), TEXT("")).Replace(TEXT(" "), TEXT(""));
		if (Name.Equals(SkillName.Replace(TEXT("'"), TEXT("")).Replace(TEXT(" "), TEXT("")), ESearchCase::IgnoreCase))
		{
			ServerBuySkill(static_cast<uint8>(Index));
			return;
		}
	}
	ClientReceiveChat(ENAChatChannel::System, FString(), TEXT("Skills: Vitality, Endurance, HuntersEye, Survivalist, PowerMastery (or press K)."));
}

void ANAPlayerController::ServerBuySkill_Implementation(uint8 Skill)
{
	ANAPlayerState* State = GetPlayerState<ANAPlayerState>();
	if (!State || Skill >= ANAPlayerState::SkillCount)
	{
		return;
	}
	const ENASkill Which = static_cast<ENASkill>(Skill);
	if (!State->TryBuySkill(Which))
	{
		ClientReceiveChat(ENAChatChannel::System, FString(), State->GetSkillPoints() <= 0 ? TEXT("No skill points. Earn XP to level up.") : TEXT("That skill is already maxed."));
		return;
	}
	ClientReceiveChat(ENAChatChannel::System, FString(), FString::Printf(TEXT("%s is now rank %d."), *ANAPlayerState::SkillName(Which), State->GetSkillRank(Which)));
	if (ANACharacterBase* Body = Cast<ANACharacterBase>(GetPawn()))
	{
		Body->ApplyProgression();
	}
}

void ANAPlayerController::ClientXPGained_Implementation(int32 Amount, const FString& Reason)
{
	LastXPTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	LastXPAmount = Amount;
	LastXPReason = Reason;
}

void ANAPlayerController::ServerAdminCommand_Implementation(ENAAdminCommand Command, const FString& Arg1, const FString& Arg2, FVector_NetQuantize Location)
{
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->HandleAdminCommand(this, Command, Arg1, Arg2, Location);
	}
}
