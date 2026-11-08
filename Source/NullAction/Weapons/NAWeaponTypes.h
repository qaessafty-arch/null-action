#pragma once

#include "CoreMinimal.h"
#include "Weapons/NAWeapon.h"
#include "NAWeaponTypes.generated.h"

/** Slot 1. Accurate semi-auto sidearm; every player spawns with one. */
UCLASS()
class NULLACTION_API ANAWeapon_Pistol : public ANAWeapon
{
	GENERATED_BODY()
public:
	ANAWeapon_Pistol();
};

/** Slot 2. Full-auto rifle, controllable recoil. */
UCLASS()
class NULLACTION_API ANAWeapon_AssaultRifle : public ANAWeapon
{
	GENERATED_BODY()
public:
	ANAWeapon_AssaultRifle();
};

/** Slot 3. Nine pellets per shell; devastating up close, weak at range. */
UCLASS()
class NULLACTION_API ANAWeapon_Shotgun : public ANAWeapon
{
	GENERATED_BODY()
public:
	ANAWeapon_Shotgun();
};

/** Slot 4. Bolt-action; one headshot kills an unarmored target. Deep zoom when aiming. */
UCLASS()
class NULLACTION_API ANAWeapon_SniperRifle : public ANAWeapon
{
	GENERATED_BODY()
public:
	ANAWeapon_SniperRifle();
};
