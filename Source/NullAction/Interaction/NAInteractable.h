#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NAInteractable.generated.h"

class ANACharacterBase;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UNAInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything the player can use with the interact key: pickups now; doors, containers, crafting
 * stations, vehicles and mounts later. The client only asks; the server validates distance and
 * calls Interact.
 */
class NULLACTION_API INAInteractable
{
	GENERATED_BODY()

public:
	/** Text shown on screen, e.g. "Pick up Shotgun". */
	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const { return FText::GetEmpty(); }

	virtual bool CanInteract(const ANACharacterBase* User) const { return true; }

	/** Server only. */
	virtual void Interact(ANACharacterBase* User) {}
};
