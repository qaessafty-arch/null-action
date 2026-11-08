#include "Abilities/NAAbilityComponent.h"

#include "Character/NAPlayerCharacter.h"
#include "Combat/NAHealthComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/NATracer.h"

namespace NAAbility
{
	struct FSpec { float Cost; float Cooldown; float Duration; };
	FSpec SpecFor(ENAArchetype Archetype)
	{
		switch (Archetype)
		{
		case ENAArchetype::Phantom:  return { 50.f, 20.f, 8.f };
		case ENAArchetype::Hunter:   return { 35.f, 15.f, 10.f };
		case ENAArchetype::Storm:    return { 60.f, 18.f, 0.6f };
		case ENAArchetype::Guardian: return { 60.f, 25.f, 8.f };
		default:                     return { 1000.f, 1.f, 0.f };
		}
	}
	constexpr float StrikeRadius = 350.f;
	constexpr float StrikeDamage = 70.f;
	constexpr float StrikeRange = 4000.f;
	constexpr float FieldRadius = 600.f;
	constexpr float FieldHealPerPulse = 8.f;
}

UNAAbilityComponent::UNAAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	SetIsReplicatedByDefault(true);
}

void UNAAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UNAAbilityComponent, Energy, COND_OwnerOnly);
	DOREPLIFETIME(UNAAbilityComponent, CooldownEnd);
	DOREPLIFETIME(UNAAbilityComponent, ActiveEnd);
	DOREPLIFETIME(UNAAbilityComponent, StrikeLocation);
	DOREPLIFETIME(UNAAbilityComponent, StrikeWarningEnd);
}

void UNAAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	Energy = MaxEnergy;
}

float UNAAbilityComponent::ServerTime() const
{
	const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return State ? static_cast<float>(State->GetServerWorldTimeSeconds()) : 0.f;
}

ENAArchetype UNAAbilityComponent::GetArchetype() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const ANAPlayerState* State = Pawn ? Pawn->GetPlayerState<ANAPlayerState>() : nullptr;
	return State ? State->GetArchetype() : ENAArchetype::None;
}

FString UNAAbilityComponent::AbilityName(ENAArchetype Archetype)
{
	switch (Archetype)
	{
	case ENAArchetype::Phantom:  return TEXT("Cloak");
	case ENAArchetype::Hunter:   return TEXT("Tracking Pulse");
	case ENAArchetype::Storm:    return TEXT("Lightning Strike");
	case ENAArchetype::Guardian: return TEXT("Restoration Field");
	default:                     return TEXT("None");
	}
}

float UNAAbilityComponent::GetCooldownRemaining() const { return FMath::Max(0.f, CooldownEnd - ServerTime()); }
float UNAAbilityComponent::GetActiveRemaining() const   { return FMath::Max(0.f, ActiveEnd - ServerTime()); }
bool UNAAbilityComponent::IsCloaked() const        { return GetArchetype() == ENAArchetype::Phantom && GetActiveRemaining() > 0.f; }
bool UNAAbilityComponent::IsTrackingActive() const { return GetArchetype() == ENAArchetype::Hunter && GetActiveRemaining() > 0.f; }
bool UNAAbilityComponent::IsFieldActive() const    { return GetArchetype() == ENAArchetype::Guardian && GetActiveRemaining() > 0.f; }

void UNAAbilityComponent::BreakCloak()
{
	if (GetOwner() && GetOwner()->HasAuthority() && IsCloaked())
	{
		ActiveEnd = ServerTime();
	}
}

FVector UNAAbilityComponent::GetAimPoint() const
{
	const ANACharacterBase* Character = Cast<ANACharacterBase>(GetOwner());
	if (!Character)
	{
		return FVector::ZeroVector;
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	Character->GetAimViewPoint(ViewLocation, ViewRotation);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NAAbilityAim), false, Character);
	FHitResult Hit;
	const FVector End = ViewLocation + ViewRotation.Vector() * NAAbility::StrikeRange;
	return GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, End, ECC_Visibility, Params) ? Hit.ImpactPoint : End;
}

void UNAAbilityComponent::RequestActivate()
{
	const FVector AimPoint = GetAimPoint();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ActivatePower(AimPoint);
	}
	else
	{
		ServerActivate(AimPoint);
	}
}

void UNAAbilityComponent::ServerActivate_Implementation(FVector_NetQuantize AimPoint)
{
	ActivatePower(AimPoint);
}

void UNAAbilityComponent::ActivatePower(const FVector& AimPoint)
{
	const ANACharacterBase* Character = Cast<ANACharacterBase>(GetOwner());
	const ENAArchetype Archetype = GetArchetype();
	const NAAbility::FSpec Spec = NAAbility::SpecFor(Archetype);
	const float Now = ServerTime();
	if (!Character || Character->IsDead() || Character->IsDowned() || Now < CooldownEnd || Energy < Spec.Cost)
	{
		return;
	}

	FVector Target = AimPoint;
	if (Archetype == ENAArchetype::Storm)
	{
		// Anti-cheat: the strike can't land beyond range or somewhere the caster can't see.
		const FVector Eye = Character->GetPawnViewLocation();
		if (FVector::Dist(Eye, Target) > NAAbility::StrikeRange + 100.f)
		{
			return;
		}
		FCollisionQueryParams Params(SCENE_QUERY_STAT(NAStrikeCheck), false, Character);
		FHitResult Block;
		if (GetWorld()->LineTraceSingleByChannel(Block, Eye, Target + (Eye - Target).GetSafeNormal() * 50.f, ECC_Visibility, Params))
		{
			Target = Block.ImpactPoint;
		}
		StrikeLocation = Target;
		StrikeWarningEnd = Now + Spec.Duration;
		bStrikePending = true;
	}

	Energy -= Spec.Cost;
	CooldownEnd = Now + Spec.Cooldown;
	ActiveEnd = Now + Spec.Duration;
	NextFieldPulse = Now;
}

void UNAAbilityComponent::ResolveStrike()
{
	bStrikePending = false;
	ANACharacterBase* Caster = Cast<ANACharacterBase>(GetOwner());
	if (!Caster)
	{
		return;
	}
	TArray<AActor*> Ignored;
	Ignored.Add(Caster); // Don't strike yourself.
	UGameplayStatics::ApplyRadialDamage(this, NAAbility::StrikeDamage, StrikeLocation, NAAbility::StrikeRadius, UDamageType::StaticClass(),
		Ignored, Caster, Caster->GetController(), /*bDoFullDamage*/ false, ECC_Visibility);
	MulticastLightning(StrikeLocation);
}

void UNAAbilityComponent::MulticastLightning_Implementation(FVector_NetQuantize Location)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// A jagged bolt from the sky made of tracer segments.
	FVector Top = Location + FVector(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-200.f, 200.f), 2500.f);
	for (int32 Segment = 0; Segment < 6; ++Segment)
	{
		const float Alpha = (Segment + 1) / 6.f;
		const FVector Next = Segment == 5 ? FVector(Location) : FMath::Lerp(Top, FVector(Location), Alpha) + FVector(FMath::FRandRange(-120.f, 120.f), FMath::FRandRange(-120.f, 120.f), 0.f);
		ANATracer::Spawn(GetWorld(), Top, Next);
		Top = Next;
	}
}

void UNAAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	const float Now = ServerTime();
	Energy = FMath::Min(MaxEnergy, Energy + EnergyRegenPerSecond * DeltaTime);

	if (bStrikePending && Now >= StrikeWarningEnd)
	{
		ResolveStrike();
	}

	// Restoration Field: heal pulses for the Guardian and teammates nearby.
	if (IsFieldActive() && Now >= NextFieldPulse)
	{
		NextFieldPulse = Now + 1.f;
		for (TActorIterator<ANAPlayerCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->IsDead() && !It->IsDowned() && FVector::Dist(It->GetActorLocation(), Owner->GetActorLocation()) <= NAAbility::FieldRadius)
			{
				if (UNAHealthComponent* Health = It->GetHealthComponent())
				{
					Health->Heal(NAAbility::FieldHealPerPulse);
				}
			}
		}
	}
}
