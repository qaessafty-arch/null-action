#include "Survival/NASurvivalComponent.h"

#include "Character/NACharacterBase.h"
#include "Combat/NAHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UNASurvivalComponent::UNASurvivalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	SetIsReplicatedByDefault(true);
}

void UNASurvivalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// Only the owning player needs their own needs; saves bandwidth with many players.
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, Stamina, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, Hunger, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, Thirst, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, bExhausted, COND_OwnerOnly);
}

void UNASurvivalComponent::BeginPlay()
{
	Super::BeginPlay();
	const bool bServer = GetOwner() && GetOwner()->HasAuthority();
	SetComponentTickEnabled(bServer); // Simulation runs on the server only.
	if (bServer)
	{
		Stamina = MaxStamina;
		Hunger = MaxHunger;
		Thirst = MaxThirst;
	}
}

void UNASurvivalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ANACharacterBase* Character = Cast<ANACharacterBase>(GetOwner());
	if (!Character || Character->IsDead())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	Hunger = FMath::Max(0.f, Hunger - HungerDrainPerSecond * DeltaTime);
	Thirst = FMath::Max(0.f, Thirst - ThirstDrainPerSecond * DeltaTime);

	const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	const bool bMovingOnGround = Character->GetVelocity().Size2D() > 50.f && Move && !Move->IsFalling();

	if (Character->IsSprinting() && bMovingOnGround)
	{
		Stamina = FMath::Max(0.f, Stamina - SprintDrainPerSecond * DeltaTime);
		LastStaminaUseTime = Now;
	}
	else if (Now - LastStaminaUseTime >= RegenDelaySeconds)
	{
		float Regen = RegenPerSecond;
		if (Thirst <= 0.2f * MaxThirst) { Regen *= 0.5f; }
		if (Hunger <= 0.2f * MaxHunger) { Regen *= 0.75f; }
		Stamina = FMath::Min(MaxStamina, Stamina + Regen * DeltaTime);
	}

	if (!bExhausted && Stamina <= 0.f)
	{
		bExhausted = true;
		if (Character->IsSprinting())
		{
			Character->SetDesiredGait(ENAGait::Run);
		}
	}
	else if (bExhausted && Stamina >= RecoverFromExhaustionAt)
	{
		bExhausted = false;
	}

	if (Hunger <= 0.f || Thirst <= 0.f)
	{
		const float Rate = (Hunger <= 0.f && Thirst <= 0.f) ? 2.f : 1.f;
		PendingStarvationDamage += StarvationDamagePerSecond * Rate * DeltaTime;
		if (PendingStarvationDamage >= 1.f)
		{
			const float Whole = FMath::FloorToFloat(PendingStarvationDamage);
			PendingStarvationDamage -= Whole;
			if (UNAHealthComponent* Health = Character->GetHealthComponent())
			{
				Health->ApplyDamage(Whole, ENAHitZone::Body, nullptr, nullptr, /*bBypassArmor*/ true);
			}
		}
	}
}

void UNASurvivalComponent::ConsumeStamina(float Amount)
{
	if (GetOwner() && GetOwner()->HasAuthority() && Amount > 0.f)
	{
		Stamina = FMath::Max(0.f, Stamina - Amount);
		LastStaminaUseTime = GetWorld()->GetTimeSeconds();
		if (Stamina <= 0.f)
		{
			bExhausted = true;
		}
	}
}

float UNASurvivalComponent::RestoreFood(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { return 0.f; }
	const float Old = Hunger;
	Hunger = FMath::Min(MaxHunger, Hunger + Amount);
	return Hunger - Old;
}

float UNASurvivalComponent::RestoreWater(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { return 0.f; }
	const float Old = Thirst;
	Thirst = FMath::Min(MaxThirst, Thirst + Amount);
	return Thirst - Old;
}
