#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Core/NAPlayerController.h"
#include "Core/NAPlayerState.h"
#include "AI/NAAITypes.h"
#include "Inventory/NAInventoryComponent.h"
#include "NAGameMode.generated.h"

class ANACharacterBase;
class ANATestArena;
class ANAAshShrine;
class APlayerStart;
class ANAEnemySoldier;
class ANACreature;
class UNAPlayerRecord;

/**
 * Server-only rules for NULL-ACTION sessions. Exists only on the server (listen or dedicated);
 * clients never see it, which is why authority decisions belong here and not on pawns.
 *
 * Dev convenience: in a level with no PlayerStarts, it builds the test arena, spawn points,
 * weapon/supply pickups and training dummies, so any empty level is immediately playable.
 */
UCLASS(Config = Game)
class NULLACTION_API ANAGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ANAGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** Server: chat from a player (validated, rate-limited) to the right recipients. */
	void HandleChat(ANAPlayerController* Sender, ENAChatChannel Channel, const FString& Target, const FString& Message);
	void HandleReport(ANAPlayerController* Reporter, const FString& PlayerName, const FString& Reason);

	/** Server: admin/owner command. The caller's role is checked here, never on the client. */
	void HandleAdminCommand(ANAPlayerController* Caller, ENAAdminCommand Command, const FString& Arg1, const FString& Arg2, const FVector& Location);

	/** Called by characters when they die. Players respawn after RespawnDelaySeconds. */
	void OnCharacterKilled(ANACharacterBase* Victim, AController* Killer);

	/** Called by creatures when they die; a new one returns to the den later. */
	void OnCreatureKilled(ANACreature* Creature, AController* Killer = nullptr);

	/** Applies to every enemy now and to future spawns. */
	void SetEnemyDifficulty(ENADifficulty NewDifficulty);
	ENADifficulty GetEnemyDifficulty() const { return EnemyDifficulty; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Rules")
	float RespawnDelaySeconds = 5.f;

	/** How long a dead body stays in the world. */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Rules")
	float CorpseLifeSeconds = 15.f;

	/** Read from Config/DefaultGame.ini; host can change it with the NADifficulty console command. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "NullAction|Rules")
	ENADifficulty EnemyDifficulty = ENADifficulty::Normal;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Rules")
	float EnemyRespawnSeconds = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Rules")
	float CreatureRespawnSeconds = 45.f;

	/** On a listen server (or offline) the person hosting is the owner. On a dedicated server, owners come from Saved/Server/Roles.ini. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "NullAction|Security")
	bool bHostIsOwner = true;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Chat")
	float ProximityChatRangeCm = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Chat")
	int32 MaxChatLength = 200;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Dev")
	bool bBuildTestArenaInEmptyLevels = true;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Dev")
	TSubclassOf<ANATestArena> TestArenaClass;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Dev", meta = (ClampMin = 1, ClampMax = 64))
	int32 TestArenaPlayerStarts = 8;

private:
	void EnsureTestArenaIfLevelIsEmpty();
	void SpawnArenaContent();
	void RespawnPlayer(TWeakObjectPtr<AController> Controller);
	void SpawnEnemy(int32 PostIndex);
	void SpawnCreature();
	void SpawnShrines(const FTransform& ArenaTransform);
	/** Server: a player died. Rebirth at their shrine (keeping items) or drop everything where they fell. Returns true for a rebirth. */
	bool HandlePlayerDeathItems(ANACharacterBase* Victim, APlayerController* PlayerController);
	float ServerNow() const;

	// ----- Online: identity, permissions, persistence (NAGameMode_Online.cpp) -----
	void LoadServerLists();
	void SaveServerLists() const;
	void AuditLog(const FString& Line) const;
	FString ServerFilePath(const TCHAR* FileName) const;
	ENAPlayerRole ResolveRole(const ANAPlayerController* Player) const;
	ANAPlayerController* FindPlayerByName(const FString& PlayerName) const;
	void SystemMessage(ANAPlayerController* To, const FString& Text) const;
	void BroadcastSystemMessage(const FString& Text) const;
	void LoadPlayerRecord(ANAPlayerController* Player);
	void SavePlayerRecord(AController* Player) const;
	static FString RecordSlotName(const FString& StableId);
	static FString DisplayNameOf(const AActor* Actor, const AController* Controller);

	TMap<FString, ENAPlayerRole> RolesById;
	TSet<FString> BannedIds;
	TMap<TWeakObjectPtr<APlayerController>, TArray<float>> ChatTimes;
	TMap<TWeakObjectPtr<APlayerController>, float> JoinTimes;
	FVector CreatureDenLocation = FVector::ZeroVector;

	struct FEnemyPost
	{
		FTransform SpawnTransform;
		TArray<FVector> Patrol;
	};
	TArray<FEnemyPost> EnemyPosts;

	struct FPendingRebirth
	{
		TWeakObjectPtr<ANAAshShrine> Shrine;
		TArray<FNAItemStack> Items;
	};
	TMap<TWeakObjectPtr<AController>, FPendingRebirth> PendingRebirths;
	TMap<TWeakObjectPtr<ANAEnemySoldier>, int32> EnemyPostByActor;

	UPROPERTY(Transient)
	TObjectPtr<ANATestArena> TestArena;

	/** Spawn points this game mode created for the test arena (empty in real levels). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerStart>> ArenaStarts;

	int32 NextArenaStart = 0;
	bool bCheckedLevel = false;
};
