#include "Weapons/NAWeaponTypes.h"

ANAWeapon_Pistol::ANAWeapon_Pistol()
{
	Stats.DisplayName = NSLOCTEXT("NullAction", "Pistol", "Pistol");
	Stats.Slot = 1;
	Stats.FireMode = ENAFireMode::SemiAuto;
	Stats.RoundsPerMinute = 420.f;
	Stats.Damage = 24.f;
	Stats.HeadshotMultiplier = 2.5f;
	Stats.RangeCm = 6000.f;
	Stats.HipSpreadDegrees = 2.2f;
	Stats.AimSpreadDegrees = 0.6f;
	Stats.RecoilPitchDegrees = 0.9f;
	Stats.RecoilYawDegrees = 0.4f;
	Stats.MagazineSize = 12;
	Stats.StartingReserveAmmo = 48;
	Stats.MaxReserveAmmo = 96;
	Stats.ReloadSeconds = 1.4f;
	Stats.AimFieldOfView = 65.f;
	Stats.ModelSize = FVector(26.f, 5.f, 11.f);
}

ANAWeapon_AssaultRifle::ANAWeapon_AssaultRifle()
{
	Stats.DisplayName = NSLOCTEXT("NullAction", "AssaultRifle", "Assault Rifle");
	Stats.Slot = 2;
	Stats.FireMode = ENAFireMode::FullAuto;
	Stats.RoundsPerMinute = 650.f;
	Stats.Damage = 20.f;
	Stats.HeadshotMultiplier = 2.2f;
	Stats.RangeCm = 12000.f;
	Stats.HipSpreadDegrees = 3.f;
	Stats.AimSpreadDegrees = 0.8f;
	Stats.RecoilPitchDegrees = 0.6f;
	Stats.RecoilYawDegrees = 0.35f;
	Stats.MagazineSize = 30;
	Stats.StartingReserveAmmo = 90;
	Stats.MaxReserveAmmo = 210;
	Stats.ReloadSeconds = 2.2f;
	Stats.AimFieldOfView = 55.f;
	Stats.ModelSize = FVector(72.f, 6.f, 12.f);
}

ANAWeapon_Shotgun::ANAWeapon_Shotgun()
{
	Stats.DisplayName = NSLOCTEXT("NullAction", "Shotgun", "Shotgun");
	Stats.Slot = 3;
	Stats.FireMode = ENAFireMode::SemiAuto;
	Stats.RoundsPerMinute = 75.f;
	Stats.Damage = 11.f;
	Stats.PelletsPerShot = 9;
	Stats.HeadshotMultiplier = 1.6f;
	Stats.RangeCm = 3500.f;
	Stats.HipSpreadDegrees = 7.f;
	Stats.AimSpreadDegrees = 5.f;
	Stats.RecoilPitchDegrees = 3.f;
	Stats.RecoilYawDegrees = 0.8f;
	Stats.MagazineSize = 6;
	Stats.StartingReserveAmmo = 18;
	Stats.MaxReserveAmmo = 36;
	Stats.ReloadSeconds = 2.6f;
	Stats.AimFieldOfView = 70.f;
	Stats.ModelSize = FVector(62.f, 7.f, 12.f);
	Stats.WearPerShot = 0.35f;
}

ANAWeapon_SniperRifle::ANAWeapon_SniperRifle()
{
	Stats.DisplayName = NSLOCTEXT("NullAction", "SniperRifle", "Sniper Rifle");
	Stats.Slot = 4;
	Stats.FireMode = ENAFireMode::SemiAuto;
	Stats.RoundsPerMinute = 45.f;
	Stats.Damage = 85.f;
	Stats.HeadshotMultiplier = 2.5f;
	Stats.LimbMultiplier = 0.8f;
	Stats.RangeCm = 30000.f;
	Stats.HipSpreadDegrees = 8.f;
	Stats.AimSpreadDegrees = 0.05f;
	Stats.RecoilPitchDegrees = 4.f;
	Stats.RecoilYawDegrees = 0.5f;
	Stats.MagazineSize = 5;
	Stats.StartingReserveAmmo = 15;
	Stats.MaxReserveAmmo = 30;
	Stats.ReloadSeconds = 3.f;
	Stats.AimFieldOfView = 22.f;
	Stats.AimArmLength = 120.f;
	Stats.ModelSize = FVector(96.f, 6.f, 12.f);
	Stats.WearPerShot = 0.5f;
}

ANAWeapon_HuntingBow::ANAWeapon_HuntingBow()
{
	Stats.DisplayName = NSLOCTEXT("NullAction", "HuntingBow", "Hunting Bow");
	Stats.Slot = 5;
	Stats.FireMode = ENAFireMode::SemiAuto;
	Stats.RoundsPerMinute = 60.f;
	Stats.Damage = 50.f;
	Stats.HeadshotMultiplier = 2.5f;
	Stats.LimbMultiplier = 0.7f;
	Stats.RangeCm = 7000.f;
	Stats.HipSpreadDegrees = 2.5f;
	Stats.AimSpreadDegrees = 0.25f;
	Stats.RecoilPitchDegrees = 0.4f;
	Stats.RecoilYawDegrees = 0.2f;
	Stats.MagazineSize = 1;
	Stats.StartingReserveAmmo = 12;
	Stats.MaxReserveAmmo = 40;
	Stats.ReloadSeconds = 0.8f;
	Stats.AimFieldOfView = 55.f;
	Stats.ModelSize = FVector(10.f, 4.f, 110.f); // Tall and thin, like a bow.
	Stats.NoiseLoudness = 0.06f;
	Stats.CreatureDamageMultiplier = 1.6f;
	Stats.WearPerShot = 0.15f;
	Stats.bUsesArrows = true;
}
