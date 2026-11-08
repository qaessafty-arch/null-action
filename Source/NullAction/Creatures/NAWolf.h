#pragma once

#include "CoreMinimal.h"
#include "Creatures/NACreature.h"
#include "NAWolf.generated.h"

/**
 * Grey Wolf: a smaller, faster forest predator that lives in packs in the Whispering Pines.
 * Uses the creature brain (stalk, ambush, hit-and-run, flee when hurt) plus pack calls: when one
 * wolf spots prey, packmates within 30 m join and flank. Bolder at night. Drops pelts for Fur Coats.
 */
UCLASS()
class NULLACTION_API ANAWolf : public ANACreature
{
	GENERATED_BODY()

public:
	ANAWolf(const FObjectInitializer& ObjectInitializer);

	virtual FString GetSpeciesName() const override { return TEXT("Grey Wolf"); }
	virtual int32 GetXPReward() const override { return 30; }
	virtual const FNAModelSlot& GetModelSlot() const override;

protected:
	virtual void DropLoot() override;
	virtual FName GetSkinItem() const override;
};
