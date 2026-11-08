#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NAGameMode.generated.h"

class ANATestArena;
class APlayerStart;

/**
 * Server-only rules for NULL-ACTION sessions. Exists only on the server (listen or dedicated);
 * clients never see it, which is why authority decisions belong here and not on pawns.
 *
 * Dev convenience: in a level with no PlayerStarts, it builds the test arena and player starts,
 * so any empty level (including on a dedicated server) is immediately playable.
 */
UCLASS()
class NULLACTION_API ANAGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANAGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Dev")
	bool bBuildTestArenaInEmptyLevels = true;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Dev")
	TSubclassOf<ANATestArena> TestArenaClass;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Dev", meta = (ClampMin = 1, ClampMax = 64))
	int32 TestArenaPlayerStarts = 8;

private:
	void EnsureTestArenaIfLevelIsEmpty();

	UPROPERTY(Transient)
	TObjectPtr<ANATestArena> TestArena;

	/** Spawn points this game mode created for the test arena (empty in real levels). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerStart>> ArenaStarts;

	int32 NextArenaStart = 0;
	bool bCheckedLevel = false;
};
