#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AI/NAAITypes.h"
#include "NAGameMode.generated.h"

class ANACharacterBase;
class ANATestArena;
class APlayerStart;
class ANAEnemySoldier;
class ANACreature;

/**
 * Server-only rules for NULL-ACTION sessions. Exists only on the server (listen or dedicated);
 * clients never see it, which is why authority decisions belong here and not on pawns.
 *
 * Dev convenience: in a level with no PlayerStarts, it builds the test arena, spawn points,
 * weapon/supply pickups and training dummies, so any empty level is immediately playable.
 */
UCLASS(Config = Game)
class NULLACTION_API ANAGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANAGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** Called by characters when they die. Players respawn after RespawnDelaySeconds. */
	void OnCharacterKilled(ANACharacterBase* Victim, AController* Killer);

	/** Called by creatures when they die; a new one returns to the den later. */
	void OnCreatureKilled(ANACreature* Creature);

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
	FVector CreatureDenLocation = FVector::ZeroVector;

	struct FEnemyPost
	{
		FTransform SpawnTransform;
		TArray<FVector> Patrol;
	};
	TArray<FEnemyPost> EnemyPosts;
	TMap<TWeakObjectPtr<ANAEnemySoldier>, int32> EnemyPostByActor;

	UPROPERTY(Transient)
	TObjectPtr<ANATestArena> TestArena;

	/** Spawn points this game mode created for the test arena (empty in real levels). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerStart>> ArenaStarts;

	int32 NextArenaStart = 0;
	bool bCheckedLevel = false;
};
