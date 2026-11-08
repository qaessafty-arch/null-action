#pragma once

#include "CoreMinimal.h"
#include "Creatures/NAHorse.h"
#include "NADeer.generated.h"

/**
 * Deer: skittish wild prey in the Whispering Pines. Can't be tamed. Bolts from anyone moving faster than
 * a crouch within 16 m, so hunters must sneak (crouch/prone) or use the silent bow. Drops meat and hide.
 * Reuses the horse body and herd brain at a smaller size.
 */
UCLASS()
class NULLACTION_API ANADeer : public ANAHorse
{
	GENERATED_BODY()

public:
	ANADeer(const FObjectInitializer& ObjectInitializer);

	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override { return FText::GetEmpty(); }

protected:
	virtual bool CanBeTamed() const override { return false; }
	virtual bool ShouldSpookFrom(const ANAPlayerCharacter* Player, float Distance) const override;
	virtual float GetFleeSpeed() const override { return 1150.f; }
	virtual void DropLoot() override;
};
