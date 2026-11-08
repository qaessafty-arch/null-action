#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NAWeapon.generated.h"

class ANACharacterBase;
class UStaticMeshComponent;
class UPointLightComponent;
class USoundBase;

UENUM(BlueprintType)
enum class ENAFireMode : uint8
{
	SemiAuto,
	FullAuto
};

/** Everything that makes one gun different from another. New guns are subclasses that only change these numbers. */
USTRUCT(BlueprintType)
struct FNAWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon") FText DisplayName;
	/** Inventory slot, 1-4; matches the number key that selects it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon") int32 Slot = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon") ENAFireMode FireMode = ENAFireMode::SemiAuto;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon") float RoundsPerMinute = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage") float Damage = 20.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage") float HeadshotMultiplier = 2.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage") float LimbMultiplier = 0.75f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage") int32 PelletsPerShot = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage") float RangeCm = 10000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Accuracy") float HipSpreadDegrees = 2.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Accuracy") float AimSpreadDegrees = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Accuracy") float RecoilPitchDegrees = 0.8f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Accuracy") float RecoilYawDegrees = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo") int32 MagazineSize = 12;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo") int32 StartingReserveAmmo = 36;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo") int32 MaxReserveAmmo = 96;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo") float ReloadSeconds = 1.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim") float AimFieldOfView = 60.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim") float AimArmLength = 170.f;

	/** Durability lost per shot (players' weapons only). Below 25% the gun is inaccurate; at 0 it won't fire until repaired. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Durability") float WearPerShot = 0.12f;

	/** How far AI can hear the shot, relative to a gunshot (1). The bow is nearly silent. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hunting") float NoiseLoudness = 1.f;
	/** Damage multiplier against creatures (hunting weapons hit harder). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hunting") float CreatureDamageMultiplier = 1.f;
	/** Reloaded with crafted Arrow Bundles, not Ammo Packs or supply crates. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hunting") bool bUsesArrows = false;

	/** Stand-in model size (length, width, height) in cm, until real weapon meshes are imported. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual") FVector ModelSize = FVector(30.f, 5.f, 11.f);
};

/**
 * Base for all guns. Server-authoritative: the owning client sends "I fired from here, in this direction";
 * the server checks fire rate, ammo and line of sight, traces the shot, applies damage and tells
 * everyone to play the shot effects. Ammo and reload state live on the server and replicate back.
 */
UCLASS(Abstract)
class NULLACTION_API ANAWeapon : public AActor
{
	GENERATED_BODY()

public:
	ANAWeapon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	const FNAWeaponStats& GetStats() const { return Stats; }
	int32 GetAmmoInMagazine() const { return AmmoInMagazine; }
	int32 GetReserveAmmo() const { return ReserveAmmo; }
	bool IsReloading() const { return bReloading; }
	ANACharacterBase* GetOwningCharacter() const;
	FVector GetMuzzleLocation() const;

	/** Current cone of fire in degrees (depends on aiming, moving, crouching, jumping). Used by the server and the crosshair. */
	float GetCurrentSpreadDegrees() const;

	/** Called by the owning player's input, or by AI on the server. */
	void StartFire();
	void StopFire();
	void RequestReload();

	/** Real-asset slot for this weapon type (Project Settings -> NULL-ACTION Assets -> Weapons). */
	const struct FNAWeaponAssets& GetAssets() const;

	/** Server only. */
	void OnEquipped();
	void OnUnequipped();
	int32 AddReserveAmmo(int32 Amount);

	/** 0..100. Server only to change. */
	float GetDurability() const { return Durability; }
	bool IsBroken() const { return Durability <= 0.f; }
	void Repair() { Durability = 100.f; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Weapon")
	FNAWeaponStats Stats;

	/** Optional audio; played if assigned in a Blueprint subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Weapon|Audio")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, Category = "NullAction|Weapon|Audio")
	TObjectPtr<USoundBase> ReloadSound;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Weapon")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Weapon")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Weapon")
	TObjectPtr<UStaticMeshComponent> GripMesh;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Weapon")
	TObjectPtr<USceneComponent> Muzzle;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Weapon")
	TObjectPtr<UPointLightComponent> MuzzleFlash;

	UPROPERTY(Replicated)
	int32 AmmoInMagazine = 0;

	UPROPERTY(Replicated)
	int32 ReserveAmmo = 0;

	UPROPERTY(Replicated)
	bool bReloading = false;

	UPROPERTY(Replicated)
	float Durability = 100.f;

	UFUNCTION(Server, Reliable)
	void ServerFire(FVector_NetQuantize ViewLocation, FVector_NetQuantizeNormal ViewDirection);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShotEffects(FVector_NetQuantize Start, const TArray<FVector_NetQuantize>& Ends);

	UFUNCTION(Client, Unreliable)
	void ClientHitConfirmed(float Damage, bool bHeadshot, bool bKilled);

private:
	void FireOnce();
	void ExecuteShotOnServer(const FVector& ViewLocation, const FVector& ViewDirection);
	void StartReloadOnServer();
	void FinishReload();
	void HideMuzzleFlash();
	float GetFireInterval() const;

	FTimerHandle AutoFireTimer;
	FTimerHandle ReloadTimer;
	FTimerHandle FlashTimer;
	float LastLocalFireTime = -100.f;
	float LastServerFireTime = -100.f;
};
