#include "Combat/NAHealthComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UNAHealthComponent::UNAHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNAHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNAHealthComponent, Health);
	DOREPLIFETIME(UNAHealthComponent, Armor);
	DOREPLIFETIME(UNAHealthComponent, bDead);
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
	if (!Owner || !Owner->HasAuthority() || bDead || Amount <= 0.f)
	{
		return 0.f;
	}

	float ToHealth = Amount;
	if (!bBypassArmor && Zone != ENAHitZone::Head && Armor > 0.f)
	{
		const float Absorbed = FMath::Min(Armor, Amount * ArmorAbsorption);
		Armor -= Absorbed;
		ToHealth -= Absorbed;
	}

	const float OldHealth = Health;
	Health = FMath::Clamp(Health - ToHealth, 0.f, MaxHealth);
	NotifyHealthChanged(OldHealth);

	if (Health <= 0.f)
	{
		bDead = true;
		OnDeathStateChanged.Broadcast(true);
		OnKilled.Broadcast(Owner, Instigator);
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
	if (bDead)
	{
		bDead = false;
		OnDeathStateChanged.Broadcast(false);
	}
	NotifyHealthChanged(OldHealth);
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
