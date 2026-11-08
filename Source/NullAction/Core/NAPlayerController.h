#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NAPlayerController.generated.h"

UENUM(BlueprintType)
enum class ENAChatChannel : uint8
{
	All,
	Team,
	Proximity,
	Private,
	System
};

USTRUCT()
struct FNAChatLine
{
	GENERATED_BODY()

	UPROPERTY() ENAChatChannel Channel = ENAChatChannel::All;
	UPROPERTY() FString Sender;
	UPROPERTY() FString Text;
	float LocalTime = 0.f;
};

/** Admin/owner actions. Every one is re-checked against the caller's role on the server and written to the audit log. */
UENUM()
enum class ENAAdminCommand : uint8
{
	God,
	Fly,
	Heal,
	Teleport,
	GoTo,
	Bring,
	Give,
	Spawn,
	Difficulty,
	Kick,
	Ban,
	SetRole,
	ListPlayers,
	Weather,
	Time,
	DayLength,
	Survival,
	Event,
	GiveXP
};

/**
 * NULL-ACTION player controller: chat, mute/report, scoreboard toggle and admin console commands.
 * Commands are typed in the console (~). The client only sends requests; the server decides.
 */
UCLASS()
class NULLACTION_API ANAPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// ----- Chat (console) -----
	UFUNCTION(Exec) void NAChat(const FString& Message);
	UFUNCTION(Exec) void NATeam(const FString& Message);
	UFUNCTION(Exec) void NALocal(const FString& Message);
	UFUNCTION(Exec) void NAWhisper(const FString& PlayerName, const FString& Message);
	UFUNCTION(Exec) void NAMute(const FString& PlayerName);
	UFUNCTION(Exec) void NAUnmute(const FString& PlayerName);
	UFUNCTION(Exec) void NAReport(const FString& PlayerName, const FString& Reason);
	/** Pick your class: Phantom, Hunter, Storm or Guardian (each has one power on Q). */
	UFUNCTION(Exec) void NAClass(const FString& ClassName);

	// ----- Admin / owner (console; server checks permission) -----
	UFUNCTION(Exec) void NAGod();
	UFUNCTION(Exec) void NAFly();
	UFUNCTION(Exec) void NAHeal();
	UFUNCTION(Exec) void NATeleport(float X, float Y, float Z);
	UFUNCTION(Exec) void NAGoto(const FString& PlayerName);
	UFUNCTION(Exec) void NABring(const FString& PlayerName);
	UFUNCTION(Exec) void NAGive(const FString& What, const FString& Count = TEXT(""));
	UFUNCTION(Exec) void NASpawn(const FString& What);
	UFUNCTION(Exec) void NADifficulty(const FString& Level);
	UFUNCTION(Exec) void NAKick(const FString& PlayerName);
	UFUNCTION(Exec) void NABan(const FString& PlayerName);
	UFUNCTION(Exec) void NASetRole(const FString& PlayerName, const FString& RoleName);
	UFUNCTION(Exec) void NAPlayers();
	UFUNCTION(Exec) void NAWeather(const FString& Weather);
	UFUNCTION(Exec) void NATime(float Hours);
	UFUNCTION(Exec) void NADayLength(float Minutes);
	UFUNCTION(Exec) void NASurvival(const FString& Level);
	UFUNCTION(Exec) void NAEvent(const FString& Which = TEXT(""));
	UFUNCTION(Exec) void NAGiveXP(int32 Amount);

	/** Spend a skill point (also from the K / skills tab). */
	UFUNCTION(Exec) void NASkill(const FString& SkillName);
	UFUNCTION(Server, Reliable) void ServerBuySkill(uint8 Skill);

	/** Server -> owning client: show "+25 XP" on the HUD. */
	UFUNCTION(Client, Unreliable) void ClientXPGained(int32 Amount, const FString& Reason);
	float GetLastXPTime() const { return LastXPTime; }
	int32 GetLastXPAmount() const { return LastXPAmount; }
	const FString& GetLastXPReason() const { return LastXPReason; }

	/** Server -> this client: one chat or system line. */
	UFUNCTION(Client, Reliable)
	void ClientReceiveChat(ENAChatChannel Channel, const FString& Sender, const FString& Text);

	const TArray<FNAChatLine>& GetChatLog() const { return ChatLog; }

protected:
	UFUNCTION(Server, Reliable)
	void ServerSetClass(const FString& ClassName);

	UFUNCTION(Server, Reliable)
	void ServerSendChat(ENAChatChannel Channel, const FString& Target, const FString& Message);

	UFUNCTION(Server, Reliable)
	void ServerReport(const FString& PlayerName, const FString& Reason);

	UFUNCTION(Server, Reliable)
	void ServerAdminCommand(ENAAdminCommand Command, const FString& Arg1, const FString& Arg2, FVector_NetQuantize Location);

private:
	void SendAdmin(ENAAdminCommand Command, const FString& Arg1 = FString(), const FString& Arg2 = FString(), const FVector& Location = FVector::ZeroVector);

	TArray<FNAChatLine> ChatLog;
	TSet<FString> MutedNames;

private:
	float LastXPTime = -100.f;
	int32 LastXPAmount = 0;
	FString LastXPReason;
};
