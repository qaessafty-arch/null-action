// Downed players: teammates revive them with steady E taps (a CPR-like rhythm) or carry them to safety.
#include "Character/NAPlayerCharacter.h"

#include "Abilities/NAAbilityComponent.h"
#include "Combat/NAHealthComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Weapons/NAWeapon.h"
#include "Components/CapsuleComponent.h"
#include "Core/NAPlayerState.h"
#include "Creatures/NAHorse.h"
#include "Core/NAGameMode.h"
#include "Creatures/NACreature.h"
#include "Creatures/NABoss.h"
#include "Survival/NASurvivalComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"

namespace NARescue
{
	constexpr float MinTapGap = 0.25f;       // Faster than this is mashing: ignored.
	constexpr float MaxTapGap = 1.4f;        // Slower than this breaks the rhythm.
	constexpr float ProgressPerTap = 0.08f;  // ~13 steady taps.
	constexpr float FieldTapBonus = 1.6f;    // Inside a Guardian's Restoration Field.
	constexpr float DecayPerSecond = 0.12f;  // Progress fades when nobody is helping.
	constexpr float ReviveHealthFraction = 0.3f;
	constexpr float RecoverySeconds = 20.f;
	constexpr float CarrySpeedMultiplier = 0.6f;
	constexpr float WeakSpeedMultiplier = 0.85f;
}

void ANAPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ApplyProgression();
}

void ANAPlayerCharacter::ApplyProgression()
{
	const ANAPlayerState* State = GetPlayerState<ANAPlayerState>();
	UNAHealthComponent* Health = GetHealthComponent();
	if (!HasAuthority() || !State || !Health || IsDead())
	{
		return;
	}
	const float Wanted = 100.f + 10.f * State->GetSkillRank(ENASkill::Vitality);
	if (!FMath::IsNearlyEqual(Health->GetMaxHealth(), Wanted))
	{
		Health->SetMaxHealthKeepFraction(Wanted);
	}
}

float ANAPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Dealt = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	// Heavy hits and animal bites can open a bleeding wound.
	if (Dealt > 0.f && !IsDead() && SurvivalComponent)
	{
		const bool bBite = DamageCauser && (DamageCauser->IsA<ANACreature>() || DamageCauser->IsA<ANABoss>());
		const float Chance = bBite ? 0.5f : (Dealt >= 20.f ? 0.3f : 0.f);
		if (FMath::FRand() < Chance)
		{
			SurvivalComponent->StartBleeding();
		}
	}
	return Dealt;
}

void ANAPlayerCharacter::SetMountedHorse(ANAHorse* Horse)
{
	MountedHorse = Horse;
}

ANAHorse* ANAPlayerCharacter::GetMountedHorse() const
{
	return MountedHorse.Get();
}

float ANAPlayerCharacter::ServerTimeNow() const
{
	const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return State ? static_cast<float>(State->GetServerWorldTimeSeconds()) : 0.f;
}

bool ANAPlayerCharacter::IsRecovering() const
{
	return RecoveringUntil > ServerTimeNow();
}

bool ANAPlayerCharacter::IsCloaked() const
{
	return AbilityComponent && AbilityComponent->IsCloaked();
}

float ANAPlayerCharacter::GetExtraSpeedMultiplier() const
{
	float Multiplier = 1.f;
	if (CarriedPlayer)
	{
		Multiplier *= NARescue::CarrySpeedMultiplier;
	}
	if (IsRecovering())
	{
		Multiplier *= NARescue::WeakSpeedMultiplier;
	}
	const ANAPlayerState* State = GetPlayerState<ANAPlayerState>();
	if (State && State->GetAshTouchedUntil() > ServerTimeNow())
	{
		Multiplier *= NARescue::WeakSpeedMultiplier; // Ash-touched after a rebirth.
	}
	return Multiplier;
}

// ----- Interaction: other players use E on a downed teammate -------------------------------------

FText ANAPlayerCharacter::GetInteractionPrompt(const ANACharacterBase* User) const
{
	return FText::FromString(FString::Printf(TEXT("Tap E steadily to revive (%d%%)   |   Crouch + E to carry"),
		FMath::RoundToInt(ReviveProgress * 100.f)));
}

bool ANAPlayerCharacter::CanInteract(const ANACharacterBase* User) const
{
	return IsDowned() && !IsDead() && !CarriedBy && User && User != this && !User->IsDowned() && !User->IsDead();
}

void ANAPlayerCharacter::Interact(ANACharacterBase* User)
{
	ANAPlayerCharacter* Helper = Cast<ANAPlayerCharacter>(User);
	if (!Helper || !CanInteract(User) || Helper->CarriedPlayer)
	{
		return;
	}
	if (Helper->bIsCrouched)
	{
		Helper->StartCarrying(this);
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float Gap = Now - LastReviveTapTime;
	LastReviveTapTime = Now;
	if (Gap < NARescue::MinTapGap)
	{
		return; // Mashing doesn't help; keep a steady rhythm.
	}
	float Step = Gap <= NARescue::MaxTapGap ? NARescue::ProgressPerTap : NARescue::ProgressPerTap * 0.5f;

	// Reviving goes faster inside any active Restoration Field.
	for (TActorIterator<ANAPlayerCharacter> It(GetWorld()); It; ++It)
	{
		const UNAAbilityComponent* Ability = It->GetAbilityComponent();
		if (Ability && Ability->IsFieldActive() && FVector::Dist(It->GetActorLocation(), GetActorLocation()) <= 600.f)
		{
			Step *= NARescue::FieldTapBonus;
			break;
		}
	}

	ReviveProgress = FMath::Min(1.f, ReviveProgress + Step);
	if (ReviveProgress >= 1.f)
	{
		CompleteRevive();
		if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
		{
			GameMode->AwardXP(Helper->GetController(), 40, TEXT("Revived a teammate"));
		}
	}
}

void ANAPlayerCharacter::CompleteRevive()
{
	ReviveProgress = 0.f;
	RecoveringUntil = ServerTimeNow() + NARescue::RecoverySeconds;
	if (UNAHealthComponent* Health = GetHealthComponent())
	{
		Health->ReviveFromDowned(NARescue::ReviveHealthFraction);
	}
	ApplyGait();
}

// ----- Carrying ----------------------------------------------------------------------------------

void ANAPlayerCharacter::StartCarrying(ANAPlayerCharacter* Downed)
{
	if (!HasAuthority() || !Downed || Downed->CarriedBy || CarriedPlayer)
	{
		return;
	}
	UnCrouch();
	SetAiming(false);
	if (GetCurrentWeapon()) { GetCurrentWeapon()->StopFire(); }
	CarriedPlayer = Downed;
	Downed->CarriedBy = this;
	Downed->ReviveProgress = 0.f;
	Downed->ApplyCarriedState();
	ApplyGait();
}

void ANAPlayerCharacter::ServerDropCarried_Implementation()
{
	DropCarried();
}

void ANAPlayerCharacter::DropCarried()
{
	if (!HasAuthority() || !CarriedPlayer)
	{
		return;
	}
	ANAPlayerCharacter* Dropped = CarriedPlayer;
	CarriedPlayer = nullptr;
	Dropped->CarriedBy = nullptr;
	Dropped->ApplyCarriedState();

	// Put them down just behind the carrier, on the ground.
	FVector Place = GetActorLocation() - GetActorForwardVector() * 90.f;
	FHitResult Ground;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NADropCarried), false, this);
	Params.AddIgnoredActor(Dropped);
	if (GetWorld()->LineTraceSingleByChannel(Ground, Place + FVector(0.f, 0.f, 100.f), Place - FVector(0.f, 0.f, 400.f), ECC_Visibility, Params))
	{
		Place = Ground.ImpactPoint + FVector(0.f, 0.f, Dropped->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	}
	Dropped->TeleportTo(Place, Dropped->GetActorRotation(), false, true);
	ApplyGait();
}

void ANAPlayerCharacter::OnRep_CarriedBy()
{
	ApplyCarriedState();
}

void ANAPlayerCharacter::ApplyCarriedState()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (CarriedBy)
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		AttachToActor(CarriedBy, FAttachmentTransformRules::KeepWorldTransform);
		SetActorRelativeLocation(FVector(-10.f, 0.f, 95.f)); // Over the shoulder.
		SetActorRelativeRotation(FRotator(0.f, 90.f, 0.f));
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		if (!IsDead())
		{
			Move->SetMovementMode(MOVE_Walking);
		}
	}
}

// ----- Per-frame upkeep --------------------------------------------------------------------------

void ANAPlayerCharacter::UpdateRescue(float DeltaSeconds)
{
	if (HasAuthority())
	{
		// Revive progress fades when nobody keeps the rhythm.
		if (ReviveProgress > 0.f && GetWorld()->GetTimeSeconds() - LastReviveTapTime > NARescue::MaxTapGap * 1.5f)
		{
			ReviveProgress = FMath::Max(0.f, ReviveProgress - NARescue::DecayPerSecond * DeltaSeconds);
		}
		if (!IsDowned() && ReviveProgress > 0.f)
		{
			ReviveProgress = 0.f;
		}
		// Drop the carried teammate if either of us is no longer in a state to carry / be carried.
		if (CarriedPlayer && (IsDead() || IsDowned() || CarriedPlayer->IsDead() || !CarriedPlayer->IsDowned() || IsSwimming()))
		{
			DropCarried();
		}
	}

	// Temporary slowdowns expire on their own; re-apply speed limits when the multiplier changes.
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now >= NextSpeedRefresh)
	{
		NextSpeedRefresh = Now + 0.5f;
		const float Multiplier = GetExtraSpeedMultiplier();
		if (!FMath::IsNearlyEqual(Multiplier, LastSpeedMultiplier))
		{
			LastSpeedMultiplier = Multiplier;
			ApplyGait();
		}
	}

	// Cloak: other players can't see the Phantom (component-level hiding, so nothing replicates or
	// stops being network-relevant); the Phantom still sees themselves.
	const bool bHide = IsCloaked() && !IsLocallyControlled();
	if (bHide || bWasCloaked)
	{
		bWasCloaked = bHide;
		auto HideParts = [bHide](AActor* Actor)
		{
			TInlineComponentArray<UPrimitiveComponent*> Parts(Actor);
			for (UPrimitiveComponent* Part : Parts)
			{
				Part->SetHiddenInGame(bHide);
			}
		};
		HideParts(this);
		for (ANAWeapon* Weapon : GetWeapons())
		{
			if (Weapon) { HideParts(Weapon); }
		}
	}
}

// ----- Hooks from the base character -------------------------------------------------------------

void ANAPlayerCharacter::OnTookDamageHook()
{
	if (AbilityComponent)
	{
		AbilityComponent->BreakCloak();
	}
}

void ANAPlayerCharacter::OnFiredWeaponHook()
{
	if (AbilityComponent)
	{
		AbilityComponent->BreakCloak();
	}
}

void ANAPlayerCharacter::OnDownedChanged(bool bNowDowned)
{
	Super::OnDownedChanged(bNowDowned);
	if (bNowDowned && HasAuthority())
	{
		if (ANAHorse* Horse = MountedHorse.Get())
		{
			Horse->Dismount(); // Thrown from the saddle.
		}
	}
	if (bNowDowned)
	{
		if (bBuildMode) { StopBuilding(); }
		bInventoryOpen = false;
		if (HasAuthority() && CarriedPlayer)
		{
			DropCarried();
		}
	}
	else if (HasAuthority() && CarriedBy)
	{
		if (CarriedBy) { CarriedBy->DropCarried(); }
	}
}
