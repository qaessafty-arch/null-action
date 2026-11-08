#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NAGameMode.generated.h"

class ANACharacterBase;
class ANATestArena;
class APlayerStart;

/**
 * Server-only rules for NULL-ACTION sessions. Exists only on the server (listen or dedicated);
 * clients never see it, which is why authority decisions belong here and not on pawns.
 *
 * Dev convenience: in a level with no PlayerStarts, it builds the test arena, spawn points,
 * weapon/supply pickups and training dummies, so any empty level is immediately playable.
 */
UCLASS()
class NULLACTION_API ANAGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANAGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** Called by characters when they die. Players respawn after RespawnDelaySeconds. */
	void OnCharacterKilled(ANACharacterBase* Victim, AController* Killer);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Rules")
	float RespawnDelaySeconds = 5.f;

	/** How long a dead body stays in the world. */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Rules")
	float CorpseLifeSeconds = 15.f;

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

	UPROPERTY(Transient)
	TObjectPtr<ANATestArena> TestArena;

	/** Spawn points this game mode created for the test arena (empty in real levels). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerStart>> ArenaStarts;

	int32 NextArenaStart = 0;
	bool bCheckedLevel = false;
};
