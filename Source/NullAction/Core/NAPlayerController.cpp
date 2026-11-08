#include "Core/NAPlayerController.h"

#include "Core/NAGameMode.h"
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
void ANAPlayerController::NAGive(const FString& Weapon)             { SendAdmin(ENAAdminCommand::Give, Weapon); }
void ANAPlayerController::NASpawn(const FString& What)              { SendAdmin(ENAAdminCommand::Spawn, What); }
void ANAPlayerController::NADifficulty(const FString& Level)        { SendAdmin(ENAAdminCommand::Difficulty, Level); }
void ANAPlayerController::NAKick(const FString& PlayerName)         { SendAdmin(ENAAdminCommand::Kick, PlayerName); }
void ANAPlayerController::NABan(const FString& PlayerName)          { SendAdmin(ENAAdminCommand::Ban, PlayerName); }
void ANAPlayerController::NASetRole(const FString& PlayerName, const FString& RoleName) { SendAdmin(ENAAdminCommand::SetRole, PlayerName, RoleName); }
void ANAPlayerController::NAPlayers()                               { SendAdmin(ENAAdminCommand::ListPlayers); }

void ANAPlayerController::ServerAdminCommand_Implementation(ENAAdminCommand Command, const FString& Arg1, const FString& Arg2, FVector_NetQuantize Location)
{
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->HandleAdminCommand(this, Command, Arg1, Arg2, Location);
	}
}
