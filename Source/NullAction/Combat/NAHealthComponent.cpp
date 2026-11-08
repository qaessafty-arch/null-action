#include "Combat/NAHealthComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UNAHealthComponent::UNAHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // Only ticks while downed (bleeding out).
	SetIsReplicatedByDefault(true);
}

void UNAHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNAHealthComponent, Health);
	DOREPLIFETIME(UNAHealthComponent, Armor);
	DOREPLIFETIME(UNAHealthComponent, bDead);
	DOREPLIFETIME(UNAHealthComponent, bDowned);
}

void UNAHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Health = MaxHealth;
		Armor = FMath::Clamp(StartingArmor, 0.f, MaxArmor);
	}
}

float UNAHealthComponent::ApplyDamage(float Amount, ENAHitZone Zone, AController* Instigator, AActor* Causer, bool bBypassArmor)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || bDead || Amount <= 0.f || bInvulnerable)
	{
		return 0.f;
	}

	float ToHealth = Amount;
	if (!bDowned && !bBypassArmor && Zone != ENAHitZone::Head && Armor > 0.f)
	{
		const float Absorbed = FMath::Min(Armor, Amount * ArmorAbsorption);
		Armor -= Absorbed;
		ToHealth -= Absorbed;
	}

	const float OldHealth = Health;
	Health = FMath::Clamp(Health - ToHealth, 0.f, MaxHealth);

	if (Health <= 0.f && bCanBeDowned && !bDowned)
	{
		// Go down instead of dying: health becomes a bleed-out pool.
		bDowned = true;
		DownedBy = Instigator;
		Health = MaxHealth * DownedPoolFraction;
		SetComponentTickEnabled(true);
		NotifyHealthChanged(OldHealth);
		OnDownedStateChanged.Broadcast(true);
		return OldHealth;
	}

	NotifyHealthChanged(OldHealth);
	if (Health <= 0.f)
	{
		Die(Instigator ? Instigator : DownedBy.Get());
	}
	return OldHealth - Health;
}

float UNAHealthComponent::Heal(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bDead || Amount <= 0.f)
	{
		return 0.f;
	}
	const float OldHealth = Health;
	Health = FMath::Min(Health + Amount, MaxHealth);
	NotifyHealthChanged(OldHealth);
	return Health - OldHealth;
}

float UNAHealthComponent::AddArmor(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bDead || Amount <= 0.f)
	{
		return 0.f;
	}
	const float OldArmor = Armor;
	Armor = FMath::Min(Armor + Amount, MaxArmor);
	return Armor - OldArmor;
}

void UNAHealthComponent::Revive(float HealthFraction)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	const float OldHealth = Health;
	Health = FMath::Clamp(HealthFraction, 0.01f, 1.f) * MaxHealth;
	Armor = FMath::Clamp(StartingArmor, 0.f, MaxArmor);
	if (bDowned)
	{
		bDowned = false;
		SetComponentTickEnabled(false);
		OnDownedStateChanged.Broadcast(false);
	}
	if (bDead)
	{
		bDead = false;
		OnDeathStateChanged.Broadcast(false);
	}
	NotifyHealthChanged(OldHealth);
}

void UNAHealthComponent::Die(AController* Killer)
{
	const bool bWasDowned = bDowned;
	bDowned = false;
	bDead = true;
	SetComponentTickEnabled(false);
	if (bWasDowned)
	{
		OnDownedStateChanged.Broadcast(false);
	}
	OnDeathStateChanged.Broadcast(true);
	OnKilled.Broadcast(GetOwner(), Killer);
}

void UNAHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bDowned || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	// Bleeding out.
	const float OldHealth = Health;
	Health = FMath::Max(0.f, Health - BleedOutPerSecond * DeltaTime);
	if (FMath::FloorToInt(OldHealth) != FMath::FloorToInt(Health))
	{
		OnHealthChanged.Broadcast(Health, Health - OldHealth);
	}
	if (Health <= 0.f && !bInvulnerable)
	{
		Die(DownedBy.Get());
	}
}

void UNAHealthComponent::ReviveFromDowned(float HealthFraction)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bDowned)
	{
		return;
	}
	const float OldHealth = Health;
	bDowned = false;
	SetComponentTickEnabled(false);
	Health = FMath::Clamp(HealthFraction, 0.05f, 1.f) * MaxHealth;
	OnDownedStateChanged.Broadcast(false);
	NotifyHealthChanged(OldHealth);
}

void UNAHealthComponent::OnRep_Downed()
{
	OnDownedStateChanged.Broadcast(bDowned);
}

void UNAHealthComponent::OnRep_Health(float OldHealth)
{
	NotifyHealthChanged(OldHealth);
}

void UNAHealthComponent::OnRep_Dead()
{
	OnDeathStateChanged.Broadcast(bDead);
}

void UNAHealthComponent::NotifyHealthChanged(float OldHealth)
{
	if (Health < OldHealth && GetWorld())
	{
		LastDamageTime = GetWorld()->GetTimeSeconds();
	}
	OnHealthChanged.Broadcast(Health, Health - OldHealth);
}
