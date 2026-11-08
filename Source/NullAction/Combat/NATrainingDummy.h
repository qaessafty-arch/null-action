#pragma once

#include "CoreMinimal.h"
#include "Character/NACharacterBase.h"
#include "NATrainingDummy.generated.h"

/**
 * Stationary target for testing weapons: same body, hit zones and health rules as players,
 * no controller. Falls over when killed and stands back up after a few seconds.
 */
UCLASS()
class NULLACTION_API ANATrainingDummy : public ANACharacterBase
{
	GENERATED_BODY()

public:
	ANATrainingDummy(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void OnKilled(AController* Killer) override;

	UPROPERTY(EditAnywhere, Category = "NullAction|Dummy")
	float ReviveDelaySeconds = 3.f;

private:
	void ReviveDummy();
	FTimerHandle ReviveTimer;
};
