#include "Survival/NASurvivalComponent.h"

#include "Character/NACharacterBase.h"
#include "Combat/NAHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "World/NAWorldEnvironment.h"
#include "Building/NABuildPiece.h"
#include "Core/NAGameMode.h"
#include "Core/NAPlayerState.h"

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
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, Oxygen, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, BodyTemperature, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, Fatigue, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, bBleeding, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, PoisonSeconds, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, bInfected, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, bRebreather, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNASurvivalComponent, bFurCoat, COND_OwnerOnly);
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
		Oxygen = MaxOxygen;
		BodyTemperature = NormalTemperature;
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
	const float Scale = SurvivalScale();
	const float SurvivalistCut = 1.f - 0.12f * SkillRank(static_cast<uint8>(ENASkill::Survivalist));
	const float EnduranceBoost = 1.f + 0.12f * SkillRank(static_cast<uint8>(ENASkill::Endurance));
	const float PoisonThirst = PoisonSeconds > 0.f ? 2.f : 1.f;
	Hunger = FMath::Max(0.f, Hunger - HungerDrainPerSecond * Scale * SurvivalistCut * DeltaTime);
	Thirst = FMath::Max(0.f, Thirst - ThirstDrainPerSecond * Scale * SurvivalistCut * PoisonThirst * DeltaTime);

	// Fatigue: full to empty in ~50 minutes awake (faster when sprinting); resting by a fire restores it.
	const bool bByFire = ANABuildPiece::FindCampfireNear(GetWorld(), Character->GetActorLocation(), 450.f) != nullptr;
	if (bByFire && Character->GetVelocity().Size2D() < 20.f)
	{
		Fatigue = FMath::Min(100.f, Fatigue + 2.5f * DeltaTime);
	}
	else
	{
		Fatigue = FMath::Max(0.f, Fatigue - (100.f / (50.f * 60.f)) * Scale * (Character->IsSprinting() ? 1.5f : 1.f) * DeltaTime);
	}

	// Conditions.
	PoisonSeconds = FMath::Max(0.f, PoisonSeconds - DeltaTime);
	if (bBleeding && !bInfected && Now - BleedingStartTime > 90.f)
	{
		bInfected = true; // An untreated wound goes bad.
	}

	const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	const bool bMovingOnGround = Character->GetVelocity().Size2D() > 50.f && Move && !Move->IsFalling();

	if (Character->IsSprinting() && bMovingOnGround)
	{
		Stamina = FMath::Max(0.f, Stamina - SprintDrainPerSecond / EnduranceBoost * DeltaTime);
		LastStaminaUseTime = Now;
	}
	else if (Now - LastStaminaUseTime >= RegenDelaySeconds)
	{
		float Regen = RegenPerSecond * EnduranceBoost;
		if (Fatigue < 25.f) { Regen *= Fatigue <= 0.f ? 0.3f : 0.6f; }
		if (PoisonSeconds > 0.f) { Regen *= 0.6f; }
		if (bInfected) { Regen *= 0.5f; }
		if (Thirst <= 0.2f * MaxThirst) { Regen *= 0.5f; }
		if (Hunger <= 0.2f * MaxHunger) { Regen *= 0.75f; }
		if (BodyTemperature < HypothermiaTemperature) { Regen *= 0.5f; }
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

	// Breath: drains with your head underwater, recovers at the surface.
	if (Character->IsHeadUnderwater())
	{
		Oxygen = FMath::Max(0.f, Oxygen - OxygenDrainPerSecond * (bRebreather ? 0.25f : 1.f) * DeltaTime); // A rebreather lasts 4x longer.
	}
	else
	{
		Oxygen = FMath::Min(MaxOxygen, Oxygen + OxygenRecoverPerSecond * DeltaTime);
	}

	// Temperature: water and cold weather chill you; mild weather warms you back up; sprinting warms a little.
	const ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this);
	const float Ambient = Environment ? Environment->GetAmbientTemperature() : 20.f;
	const bool bWetFromRain = Environment && Environment->GetBlendedProfile().Rain > 0.3f;
	if (Character->IsInWater())
	{
		BodyTemperature = FMath::Max(30.f, BodyTemperature - WaterCoolingPerSecond * DeltaTime);
	}
	else if (bByFire)
	{
		// Sitting by a campfire warms you quickly, whatever the weather.
		BodyTemperature = FMath::Min(NormalTemperature, BodyTemperature + 0.25f * DeltaTime);
	}
	else if (Ambient < 15.f)
	{
		float Cooling = (15.f - Ambient) * 0.0015f * (bWetFromRain ? 1.5f : 1.f);
		if (Character->IsSprinting()) { Cooling *= 0.5f; }
		if (bFurCoat) { Cooling *= 0.5f; }
		BodyTemperature = FMath::Max(30.f, BodyTemperature - Cooling * DeltaTime);
	}
	else if (BodyTemperature < NormalTemperature)
	{
		const float Warm = WarmingPerSecond * (Character->IsSprinting() ? 1.6f : 1.f);
		BodyTemperature = FMath::Min(NormalTemperature, BodyTemperature + Warm * DeltaTime);
	}

	float EnvironmentDps = 0.f;
	if (Oxygen <= 0.f) { EnvironmentDps += DrowningDamagePerSecond; }
	if (BodyTemperature < SevereHypothermiaTemperature) { EnvironmentDps += 1.f; }
	if (bBleeding) { EnvironmentDps += 0.6f; }
	if (PoisonSeconds > 0.f) { EnvironmentDps += 0.5f; }
	if (bInfected) { EnvironmentDps += 0.15f; }
	if (EnvironmentDps > 0.f)
	{
		PendingEnvironmentDamage += EnvironmentDps * DeltaTime;
		if (PendingEnvironmentDamage >= 1.f)
		{
			const float Whole = FMath::FloorToFloat(PendingEnvironmentDamage);
			PendingEnvironmentDamage -= Whole;
			if (UNAHealthComponent* Health = Character->GetHealthComponent())
			{
				Health->ApplyDamage(Whole, ENAHitZone::Body, nullptr, nullptr, /*bBypassArmor*/ true);
			}
		}
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

float UNASurvivalComponent::SurvivalScale() const
{
	const ANAGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANAGameMode>() : nullptr;
	return GameMode ? GameMode->GetSurvivalScale() : 1.f;
}

int32 UNASurvivalComponent::SkillRank(uint8 Skill) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const ANAPlayerState* State = Pawn ? Pawn->GetPlayerState<ANAPlayerState>() : nullptr;
	return State ? State->GetSkillRank(static_cast<ENASkill>(Skill)) : 0;
}

void UNASurvivalComponent::StartBleeding()
{
	if (GetOwner() && GetOwner()->HasAuthority() && !bBleeding)
	{
		bBleeding = true;
		BleedingStartTime = GetWorld()->GetTimeSeconds();
	}
}

bool UNASurvivalComponent::StopBleeding()
{
	const bool bWas = bBleeding;
	bBleeding = false;
	return bWas;
}

void UNASurvivalComponent::Poison(float Seconds)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		PoisonSeconds = FMath::Max(PoisonSeconds, Seconds);
	}
}

bool UNASurvivalComponent::CurePoison()
{
	const bool bWas = PoisonSeconds > 0.f;
	PoisonSeconds = 0.f;
	return bWas;
}

bool UNASurvivalComponent::CureInfection()
{
	const bool bWas = bInfected;
	bInfected = false;
	return bWas;
}

float UNASurvivalComponent::Rest(float Amount)
{
	const float Old = Fatigue;
	Fatigue = FMath::Clamp(Fatigue + Amount, 0.f, 100.f);
	return Fatigue - Old;
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
