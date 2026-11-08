// Online systems for ANAGameMode: identity, roles/permissions, bans, audit log, chat, reports,
// admin commands and server-side player records. Everything here runs on the server only.

#include "Core/NAGameMode.h"

#include "NullAction.h"
#include "AI/NAEnemySoldier.h"
#include "AI/NAEnemyAIController.h"
#include "Character/NAPlayerCharacter.h"
#include "Combat/NAHealthComponent.h"
#include "Combat/NATrainingDummy.h"
#include "Core/NAPlayerRecord.h"
#include "Creatures/NACreature.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameSession.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Weapons/NAWeaponTypes.h"
#include "World/NAWorldEnvironment.h"
#include "Inventory/NAInventoryComponent.h"
#include "Inventory/NAItems.h"

namespace NAOnline
{
	const TCHAR* RoleName(ENAPlayerRole Role)
	{
		switch (Role)
		{
		case ENAPlayerRole::Player:    return TEXT("Player");
		case ENAPlayerRole::Moderator: return TEXT("Moderator");
		case ENAPlayerRole::Admin:     return TEXT("Admin");
		case ENAPlayerRole::Owner:     return TEXT("Owner");
		}
		return TEXT("Player");
	}

	bool ParseRole(const FString& Text, ENAPlayerRole& Out)
	{
		const ENAPlayerRole All[] = { ENAPlayerRole::Player, ENAPlayerRole::Moderator, ENAPlayerRole::Admin, ENAPlayerRole::Owner };
		for (const ENAPlayerRole Candidate : All)
		{
			if (Text.Equals(RoleName(Candidate), ESearchCase::IgnoreCase))
			{
				Out = Candidate;
				return true;
			}
		}
		return false;
	}

	/** Minimum role needed for each admin command. */
	ENAPlayerRole RequiredRole(ENAAdminCommand Command)
	{
		switch (Command)
		{
		case ENAAdminCommand::ListPlayers: return ENAPlayerRole::Player;
		case ENAAdminCommand::Kick:        return ENAPlayerRole::Moderator;
		case ENAAdminCommand::SetRole:
		case ENAAdminCommand::Ban:         return ENAPlayerRole::Owner;
		default:                           return ENAPlayerRole::Admin;
		}
	}

	constexpr int32 ChatBurstLimit = 5;
	constexpr float ChatBurstWindowSeconds = 5.f;
}

// ----- Files (server machine only, under Saved/Server; never packaged with the client) -----------

FString ANAGameMode::ServerFilePath(const TCHAR* FileName) const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Server"), FileName);
}

void ANAGameMode::LoadServerLists()
{
	RolesById.Reset();
	BannedIds.Reset();

	// Roles.ini lines:  <StableId>=Owner|Admin|Moderator
	TArray<FString> Lines;
	if (FFileHelper::LoadFileToStringArray(Lines, *ServerFilePath(TEXT("Roles.ini"))))
	{
		for (const FString& Raw : Lines)
		{
			FString Id, RoleText;
			const FString Line = Raw.TrimStartAndEnd();
			if (Line.IsEmpty() || Line.StartsWith(TEXT(";")) || !Line.Split(TEXT("="), &Id, &RoleText))
			{
				continue;
			}
			ENAPlayerRole ParsedRole;
			if (NAOnline::ParseRole(RoleText.TrimStartAndEnd(), ParsedRole))
			{
				RolesById.Add(Id.TrimStartAndEnd(), ParsedRole);
			}
		}
	}

	Lines.Reset();
	if (FFileHelper::LoadFileToStringArray(Lines, *ServerFilePath(TEXT("Bans.ini"))))
	{
		for (const FString& Raw : Lines)
		{
			const FString Line = Raw.TrimStartAndEnd();
			if (!Line.IsEmpty() && !Line.StartsWith(TEXT(";")))
			{
				BannedIds.Add(Line);
			}
		}
	}

	UE_LOG(LogNullAction, Log, TEXT("Loaded %d role assignments and %d bans from %s"),
		RolesById.Num(), BannedIds.Num(), *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Server")));
}

void ANAGameMode::SaveServerLists() const
{
	FString RolesText = TEXT("; NULL-ACTION server roles. <StableId>=Owner|Admin|Moderator. Server machine only.\n");
	for (const TPair<FString, ENAPlayerRole>& Pair : RolesById)
	{
		RolesText += FString::Printf(TEXT("%s=%s\n"), *Pair.Key, NAOnline::RoleName(Pair.Value));
	}
	FFileHelper::SaveStringToFile(RolesText, *ServerFilePath(TEXT("Roles.ini")));

	FString BansText = TEXT("; NULL-ACTION bans: one StableId per line. Server machine only.\n");
	for (const FString& Id : BannedIds)
	{
		BansText += Id + TEXT("\n");
	}
	FFileHelper::SaveStringToFile(BansText, *ServerFilePath(TEXT("Bans.ini")));
}

void ANAGameMode::AuditLog(const FString& Line) const
{
	const FString Stamped = FString::Printf(TEXT("[%s] %s\n"), *FDateTime::UtcNow().ToIso8601(), *Line);
	FFileHelper::SaveStringToFile(Stamped, *ServerFilePath(TEXT("AdminAudit.log")), FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(), FILEWRITE_Append);
	UE_LOG(LogNullAction, Log, TEXT("AUDIT %s"), *Line);
}

// ----- Login / logout ----------------------------------------------------------------------------

void ANAGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	LoadServerLists();
}

void ANAGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty() && UniqueId.IsValid() && BannedIds.Contains(UniqueId.ToString()))
	{
		ErrorMessage = TEXT("You are banned from this server.");
		AuditLog(FString::Printf(TEXT("Rejected banned player %s from %s"), *UniqueId.ToString(), *Address));
	}
}

void ANAGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ANAPlayerController* Player = Cast<ANAPlayerController>(NewPlayer);
	ANAPlayerState* State = NewPlayer ? NewPlayer->GetPlayerState<ANAPlayerState>() : nullptr;
	if (!Player || !State)
	{
		return;
	}

	State->SetRole(ResolveRole(Player));
	JoinTimes.Add(NewPlayer, GetWorld()->GetTimeSeconds());
	LoadPlayerRecord(Player);

	// The ID is printed so a server owner can copy it into Saved/Server/Roles.ini.
	UE_LOG(LogNullAction, Log, TEXT("Player joined: %s  id=%s  role=%s"), *State->GetPlayerName(), *State->GetStableId(), NAOnline::RoleName(State->GetRole()));
	BroadcastSystemMessage(FString::Printf(TEXT("%s joined"), *State->GetPlayerName()));
	SystemMessage(Player, FString::Printf(TEXT("Welcome to NULL-ACTION. Your role: %s. Chat: open the console (~) and type NAChat \"hello\"."),
		NAOnline::RoleName(State->GetRole())));
}

void ANAGameMode::Logout(AController* Exiting)
{
	if (const APlayerState* State = Exiting ? Exiting->PlayerState : nullptr)
	{
		BroadcastSystemMessage(FString::Printf(TEXT("%s left"), *State->GetPlayerName()));
	}
	SavePlayerRecord(Exiting);
	if (APlayerController* AsPlayer = Cast<APlayerController>(Exiting))
	{
		ChatTimes.Remove(AsPlayer);
		JoinTimes.Remove(AsPlayer);
	}
	Super::Logout(Exiting);
}

ENAPlayerRole ANAGameMode::ResolveRole(const ANAPlayerController* Player) const
{
	const ANAPlayerState* State = Player ? Player->GetPlayerState<ANAPlayerState>() : nullptr;
	if (!State)
	{
		return ENAPlayerRole::Player;
	}
	if (const ENAPlayerRole* Assigned = RolesById.Find(State->GetStableId()))
	{
		return *Assigned;
	}
	// Listen server / offline: the person running the game owns it. A remote client is never local here.
	if (bHostIsOwner && Player->IsLocalController() && GetNetMode() != NM_DedicatedServer)
	{
		return ENAPlayerRole::Owner;
	}
	return ENAPlayerRole::Player;
}

// ----- Persistence -------------------------------------------------------------------------------

FString ANAGameMode::RecordSlotName(const FString& StableId)
{
	FString Safe;
	for (const TCHAR Char : StableId)
	{
		Safe.AppendChar(FChar::IsAlnum(Char) ? Char : TEXT('_'));
	}
	return FString::Printf(TEXT("Player_%s"), *Safe.Left(80));
}

void ANAGameMode::LoadPlayerRecord(ANAPlayerController* Player)
{
	ANAPlayerState* State = Player ? Player->GetPlayerState<ANAPlayerState>() : nullptr;
	if (!State)
	{
		return;
	}
	const FString Slot = RecordSlotName(State->GetStableId());
	if (const UNAPlayerRecord* Record = Cast<UNAPlayerRecord>(UGameplayStatics::LoadGameFromSlot(Slot, 0)))
	{
		State->SetStats(Record->Kills, Record->Deaths);
		SystemMessage(Player, FString::Printf(TEXT("Welcome back. Lifetime: %d kills, %d deaths, %.0f minutes played."),
			Record->Kills, Record->Deaths, Record->PlaySeconds / 60.f));
	}
}

void ANAGameMode::SavePlayerRecord(AController* Player) const
{
	const ANAPlayerState* State = Player ? Player->GetPlayerState<ANAPlayerState>() : nullptr;
	APlayerController* AsPlayer = Cast<APlayerController>(Player);
	if (!State || !AsPlayer)
	{
		return;
	}

	const FString Slot = RecordSlotName(State->GetStableId());
	UNAPlayerRecord* Record = Cast<UNAPlayerRecord>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!Record)
	{
		Record = Cast<UNAPlayerRecord>(UGameplayStatics::CreateSaveGameObject(UNAPlayerRecord::StaticClass()));
		Record->FirstSeen = FDateTime::UtcNow();
	}

	Record->LastKnownName = State->GetPlayerName();
	Record->Kills = State->GetKills();
	Record->Deaths = State->GetDeaths();
	if (const float* Joined = JoinTimes.Find(AsPlayer))
	{
		Record->PlaySeconds += GetWorld()->GetTimeSeconds() - *Joined;
	}
	Record->LastSeen = FDateTime::UtcNow();
	UGameplayStatics::SaveGameToSlot(Record, Slot, 0);
}

// ----- Messages ----------------------------------------------------------------------------------

void ANAGameMode::SystemMessage(ANAPlayerController* To, const FString& Text) const
{
	if (To)
	{
		To->ClientReceiveChat(ENAChatChannel::System, FString(), Text);
	}
}

void ANAGameMode::BroadcastSystemMessage(const FString& Text) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		SystemMessage(Cast<ANAPlayerController>(It->Get()), Text);
	}
}

ANAPlayerController* ANAGameMode::FindPlayerByName(const FString& PlayerName) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ANAPlayerController* Player = Cast<ANAPlayerController>(It->Get());
		if (Player && Player->PlayerState && Player->PlayerState->GetPlayerName().Equals(PlayerName, ESearchCase::IgnoreCase))
		{
			return Player;
		}
	}
	return nullptr;
}

FString ANAGameMode::DisplayNameOf(const AActor* Actor, const AController* Controller)
{
	if (Controller && Controller->PlayerState)
	{
		return Controller->PlayerState->GetPlayerName();
	}
	if (Actor && Actor->IsA<ANAEnemySoldier>()) { return TEXT("Soldier"); }
	if (Actor && Actor->IsA<ANACreature>())     { return TEXT("Ash Stalker"); }
	if (Controller && Controller->GetPawn())
	{
		return DisplayNameOf(Controller->GetPawn(), nullptr);
	}
	return TEXT("the world");
}

void ANAGameMode::HandleChat(ANAPlayerController* Sender, ENAChatChannel Channel, const FString& Target, const FString& Message)
{
	if (!Sender || !Sender->PlayerState || Channel == ENAChatChannel::System)
	{
		return; // Clients can't send "system" messages.
	}

	FString Clean = Message.TrimStartAndEnd().Left(MaxChatLength);
	Clean.ReplaceInline(TEXT("\n"), TEXT(" "));
	if (Clean.IsEmpty())
	{
		return;
	}

	// Anti-spam: at most N messages per window.
	const float Now = GetWorld()->GetTimeSeconds();
	TArray<float>& Times = ChatTimes.FindOrAdd(Sender);
	Times.RemoveAll([Now](float Time) { return Now - Time > NAOnline::ChatBurstWindowSeconds; });
	if (Times.Num() >= NAOnline::ChatBurstLimit)
	{
		SystemMessage(Sender, TEXT("You're sending messages too fast."));
		return;
	}
	Times.Add(Now);

	const FString SenderName = Sender->PlayerState->GetPlayerName();
	const APawn* SenderPawn = Sender->GetPawn();

	if (Channel == ENAChatChannel::Private)
	{
		ANAPlayerController* Recipient = FindPlayerByName(Target);
		if (!Recipient)
		{
			SystemMessage(Sender, FString::Printf(TEXT("No player called %s."), *Target));
			return;
		}
		Recipient->ClientReceiveChat(Channel, SenderName, Clean);
		Sender->ClientReceiveChat(Channel, FString::Printf(TEXT("to %s"), *Target), Clean);
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ANAPlayerController* Recipient = Cast<ANAPlayerController>(It->Get());
		if (!Recipient)
		{
			continue;
		}
		if (Channel == ENAChatChannel::Proximity)
		{
			const APawn* RecipientPawn = Recipient->GetPawn();
			if (!SenderPawn || !RecipientPawn || FVector::Dist(SenderPawn->GetActorLocation(), RecipientPawn->GetActorLocation()) > ProximityChatRangeCm)
			{
				continue;
			}
		}
		// Team chat: no teams exist yet, so it reaches everyone (teams arrive with PvP modes).
		Recipient->ClientReceiveChat(Channel, SenderName, Clean);
	}
}

void ANAGameMode::HandleReport(ANAPlayerController* Reporter, const FString& PlayerName, const FString& Reason)
{
	const ANAPlayerController* Reported = FindPlayerByName(PlayerName);
	if (!Reporter || !Reporter->PlayerState || !Reported || !Reported->PlayerState)
	{
		SystemMessage(Reporter, TEXT("Report not sent: player not found."));
		return;
	}
	const ANAPlayerState* ReportedState = Reported->GetPlayerState<ANAPlayerState>();
	const FString Line = FString::Printf(TEXT("[%s] %s reported %s (%s): %s\n"), *FDateTime::UtcNow().ToIso8601(),
		*Reporter->PlayerState->GetPlayerName(), *PlayerName, ReportedState ? *ReportedState->GetStableId() : TEXT("?"), *Reason.Left(300));
	FFileHelper::SaveStringToFile(Line, *ServerFilePath(TEXT("Reports.log")), FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
	SystemMessage(Reporter, TEXT("Report sent to the server owner. Thank you."));
}

// ----- Admin / owner commands --------------------------------------------------------------------

void ANAGameMode::HandleAdminCommand(ANAPlayerController* Caller, ENAAdminCommand Command, const FString& Arg1, const FString& Arg2, const FVector& Location)
{
	ANAPlayerState* CallerState = Caller ? Caller->GetPlayerState<ANAPlayerState>() : nullptr;
	if (!CallerState)
	{
		return;
	}

	const FString CommandText = StaticEnum<ENAAdminCommand>()->GetNameStringByValue(static_cast<int64>(Command));
	const ENAPlayerRole Required = NAOnline::RequiredRole(Command);
	if (!CallerState->HasRole(Required))
	{
		AuditLog(FString::Printf(TEXT("DENIED %s (%s, %s) tried %s %s %s"), *CallerState->GetPlayerName(), *CallerState->GetStableId(),
			NAOnline::RoleName(CallerState->GetRole()), *CommandText, *Arg1, *Arg2));
		SystemMessage(Caller, FString::Printf(TEXT("Permission denied: %s needs %s."), *CommandText, NAOnline::RoleName(Required)));
		return;
	}

	if (Command != ENAAdminCommand::ListPlayers)
	{
		AuditLog(FString::Printf(TEXT("%s (%s, %s) ran %s %s %s"), *CallerState->GetPlayerName(), *CallerState->GetStableId(),
			NAOnline::RoleName(CallerState->GetRole()), *CommandText, *Arg1, *Arg2));
	}

	ANAPlayerCharacter* Body = Cast<ANAPlayerCharacter>(Caller->GetPawn());
	ANAPlayerController* TargetPlayer = Arg1.IsEmpty() ? nullptr : FindPlayerByName(Arg1);

	switch (Command)
	{
	case ENAAdminCommand::God:
		if (Body)
		{
			Body->SetGodMode(!Body->IsGodMode());
			SystemMessage(Caller, Body->IsGodMode() ? TEXT("God mode ON (invincible).") : TEXT("God mode OFF."));
		}
		break;

	case ENAAdminCommand::Fly:
		if (Body)
		{
			Body->SetAdminFlying(!Body->IsAdminFlying());
			SystemMessage(Caller, Body->IsAdminFlying() ? TEXT("Flying ON: Space up, C down.") : TEXT("Flying OFF."));
		}
		break;

	case ENAAdminCommand::Heal:
		if (Body && Body->GetHealthComponent())
		{
			Body->GetHealthComponent()->Heal(1000.f);
			Body->GetHealthComponent()->AddArmor(1000.f);
			SystemMessage(Caller, TEXT("Healed."));
		}
		break;

	case ENAAdminCommand::Teleport:
		if (Body)
		{
			Body->TeleportTo(Location, Body->GetActorRotation());
		}
		break;

	case ENAAdminCommand::GoTo:
		if (Body && TargetPlayer && TargetPlayer->GetPawn())
		{
			Body->TeleportTo(TargetPlayer->GetPawn()->GetActorLocation() + FVector(-150.f, 0.f, 50.f), Body->GetActorRotation());
		}
		else { SystemMessage(Caller, TEXT("Player not found or not spawned.")); }
		break;

	case ENAAdminCommand::Bring:
		if (Body && TargetPlayer && TargetPlayer->GetPawn())
		{
			TargetPlayer->GetPawn()->TeleportTo(Body->GetActorLocation() + Body->GetActorForwardVector() * 200.f + FVector(0.f, 0.f, 50.f), Body->GetActorRotation());
		}
		else { SystemMessage(Caller, TEXT("Player not found or not spawned.")); }
		break;

	case ENAAdminCommand::Give:
	{
		TSubclassOf<ANAWeapon> WeaponClass;
		if (Arg1.Equals(TEXT("pistol"), ESearchCase::IgnoreCase))       { WeaponClass = ANAWeapon_Pistol::StaticClass(); }
		else if (Arg1.Equals(TEXT("rifle"), ESearchCase::IgnoreCase))   { WeaponClass = ANAWeapon_AssaultRifle::StaticClass(); }
		else if (Arg1.Equals(TEXT("shotgun"), ESearchCase::IgnoreCase)) { WeaponClass = ANAWeapon_Shotgun::StaticClass(); }
		else if (Arg1.Equals(TEXT("sniper"), ESearchCase::IgnoreCase))  { WeaponClass = ANAWeapon_SniperRifle::StaticClass(); }
		if (Body && WeaponClass) { Body->GiveWeapon(WeaponClass, true); }
		else if (Body && Body->GetInventoryComponent() && NAItems::Find(FName(*Arg1)))
		{
			// Items too: NAGive Wood 50
			const int32 Count = Arg2.IsEmpty() ? 10 : FMath::Clamp(FCString::Atoi(*Arg2), 1, 999);
			Body->GetInventoryComponent()->AddItem(FName(*Arg1), Count);
		}
		else { SystemMessage(Caller, TEXT("Usage: NAGive pistol|rifle|shotgun|sniper  or  NAGive <ItemId> [count], e.g. NAGive Wood 50")); }
		break;
	}

	case ENAAdminCommand::Spawn:
	{
		if (!Body)
		{
			break;
		}
		const FVector Where = Body->GetActorLocation() + Body->GetActorForwardVector() * 600.f + FVector(0.f, 0.f, 50.f);
		const FTransform SpawnTransform(Body->GetActorRotation() + FRotator(0.f, 180.f, 0.f), Where);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (Arg1.Equals(TEXT("soldier"), ESearchCase::IgnoreCase))
		{
			if (ANAEnemySoldier* Soldier = GetWorld()->SpawnActorDeferred<ANAEnemySoldier>(ANAEnemySoldier::StaticClass(), SpawnTransform,
				nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
			{
				Soldier->Difficulty = EnemyDifficulty;
				Soldier->FinishSpawning(SpawnTransform);
			}
		}
		else if (Arg1.Equals(TEXT("creature"), ESearchCase::IgnoreCase))
		{
			if (ANACreature* Creature = GetWorld()->SpawnActorDeferred<ANACreature>(ANACreature::StaticClass(), SpawnTransform,
				nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
			{
				Creature->SetDenLocation(Where);
				Creature->FinishSpawning(SpawnTransform);
			}
		}
		else if (Arg1.Equals(TEXT("dummy"), ESearchCase::IgnoreCase))
		{
			GetWorld()->SpawnActor<ANATrainingDummy>(ANATrainingDummy::StaticClass(), SpawnTransform, Params);
		}
		else
		{
			SystemMessage(Caller, TEXT("Usage: NASpawn soldier | creature | dummy"));
		}
		break;
	}

	case ENAAdminCommand::Difficulty:
	{
		ENADifficulty NewDifficulty;
		if (NAAI::ParseDifficulty(Arg1, NewDifficulty))
		{
			SetEnemyDifficulty(NewDifficulty);
			BroadcastSystemMessage(FString::Printf(TEXT("Enemy difficulty: %s"), NAAI::DifficultyName(NewDifficulty)));
		}
		else { SystemMessage(Caller, TEXT("Usage: NADifficulty Easy | Normal | Hard | Expert")); }
		break;
	}

	case ENAAdminCommand::Kick:
	case ENAAdminCommand::Ban:
	{
		const ANAPlayerState* TargetState = TargetPlayer ? TargetPlayer->GetPlayerState<ANAPlayerState>() : nullptr;
		if (!TargetPlayer || !TargetState)
		{
			SystemMessage(Caller, TEXT("Player not found."));
			break;
		}
		if (TargetPlayer == Caller || (TargetState->HasRole(CallerState->GetRole()) && CallerState->GetRole() != ENAPlayerRole::Owner))
		{
			SystemMessage(Caller, TEXT("You can't remove someone with your own role or higher."));
			break;
		}
		if (Command == ENAAdminCommand::Ban)
		{
			BannedIds.Add(TargetState->GetStableId());
			SaveServerLists();
		}
		BroadcastSystemMessage(FString::Printf(TEXT("%s was %s."), *TargetState->GetPlayerName(), Command == ENAAdminCommand::Ban ? TEXT("banned") : TEXT("kicked")));
		if (GameSession)
		{
			GameSession->KickPlayer(TargetPlayer, FText::FromString(Command == ENAAdminCommand::Ban ? TEXT("Banned by the server owner.") : TEXT("Kicked by a moderator.")));
		}
		break;
	}

	case ENAAdminCommand::SetRole:
	{
		ENAPlayerRole NewRole;
		ANAPlayerState* TargetState = TargetPlayer ? TargetPlayer->GetPlayerState<ANAPlayerState>() : nullptr;
		if (!TargetState || !NAOnline::ParseRole(Arg2, NewRole))
		{
			SystemMessage(Caller, TEXT("Usage: NASetRole <player> Player | Moderator | Admin | Owner"));
			break;
		}
		TargetState->SetRole(NewRole);
		if (NewRole == ENAPlayerRole::Player) { RolesById.Remove(TargetState->GetStableId()); }
		else { RolesById.Add(TargetState->GetStableId(), NewRole); }
		SaveServerLists();
		BroadcastSystemMessage(FString::Printf(TEXT("%s is now %s."), *TargetState->GetPlayerName(), NAOnline::RoleName(NewRole)));
		break;
	}

	case ENAAdminCommand::Weather:
	{
		ENAWeather NewWeather;
		ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this);
		if (Environment && ANAWorldEnvironment::ParseWeather(Arg1, NewWeather))
		{
			Environment->SetWeather(NewWeather, 10.f);
			BroadcastSystemMessage(FString::Printf(TEXT("Weather changing to %s."), ANAWorldEnvironment::WeatherName(NewWeather)));
		}
		else
		{
			SystemMessage(Caller, TEXT("Usage: NAWeather Clear|Cloudy|Rain|HeavyRain|Thunderstorm|Fog|Snow|Blizzard|Windy|DustStorm"));
		}
		break;
	}

	case ENAAdminCommand::Time:
		if (ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this))
		{
			Environment->SetTimeOfDay(FCString::Atof(*Arg1));
			BroadcastSystemMessage(FString::Printf(TEXT("Time set to %s:00."), *Arg1));
		}
		break;

	case ENAAdminCommand::DayLength:
		if (ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this))
		{
			Environment->SetDayLengthMinutes(FCString::Atof(*Arg1));
			SystemMessage(Caller, FString::Printf(TEXT("A full day now lasts %s real minutes."), *Arg1));
		}
		break;

	case ENAAdminCommand::ListPlayers:
	{
		const bool bShowIds = CallerState->HasRole(ENAPlayerRole::Admin);
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			const ANAPlayerState* Listed = It->Get() ? It->Get()->GetPlayerState<ANAPlayerState>() : nullptr;
			if (!Listed)
			{
				continue;
			}
			SystemMessage(Caller, FString::Printf(TEXT("%s  [%s]  K %d / D %d  ping %.0f%s%s"), *Listed->GetPlayerName(), NAOnline::RoleName(Listed->GetRole()),
				Listed->GetKills(), Listed->GetDeaths(), Listed->GetPingInMilliseconds(),
				bShowIds ? TEXT("  id=") : TEXT(""), bShowIds ? *Listed->GetStableId() : TEXT("")));
		}
		break;
	}
	}
}
