#pragma once

#include "CoreMinimal.h"
#include "NACombatTypes.generated.h"

struct FHitResult;

UENUM(BlueprintType)
enum class ENAHitZone : uint8
{
	Body,
	Head,
	Limb
};

namespace NACombat
{
	/** Hit zone from a trace hit. Uses bone names on skeletal meshes, hit height on capsules (stand-in bodies). */
	NULLACTION_API ENAHitZone GetHitZone(const FHitResult& Hit);
}
