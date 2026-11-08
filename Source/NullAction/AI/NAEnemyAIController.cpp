#include "AI/NAEnemyAIController.h"

#include "NullAction.h"
#include "AI/NAEnemySoldier.h"
#include "AITypes.h"
#include "Character/NAPlayerCharacter.h"
#include "Combat/NAHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "TimerManager.h"
#include "Weapons/NAWeapon.h"
#include "World/NAWorldEnvironment.h"

namespace NAAIConstants
{
	constexpr float ThinkInterval = 0.1f;
	constexpr float LoseTargetSeconds = 3.f;
	constexpr float AllyAlertRadius = 3000.f;
	constexpr float CloseRange = 450.f;
}

ANAEnemyAIController::ANAEnemyAIController()
{
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 3500.f;
	SightConfig->LoseSightRadius = 4000.f;
	SightConfig->PeripheralVisionAngleDegrees = 70.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->SetMaxAge(5.f);

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 3000.f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->SetMaxAge(3.f);

	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*Perception);

	Profile = NAAI::GetDifficultyProfile(ENADifficulty::Normal);
}

void ANAEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ANAEnemySoldier* Soldier = Cast<ANAEnemySoldier>(InPawn);
	if (!Soldier)
	{
		return;
	}

	SetDifficulty(Soldier->Difficulty);
	HomeLocation = Soldier->GetActorLocation();

	Route = Soldier->PatrolPoints;
	if (Route.Num() == 0)
	{
		const float S = 500.f;
		Route = { HomeLocation + FVector(S, 0.f, 0.f), HomeLocation + FVector(S, S, 0.f), HomeLocation + FVector(0.f, S, 0.f), HomeLocation };
	}

	Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ANAEnemyAIController::OnTargetPerceptionUpdated);

	LastThinkTime = GetWorld()->GetTimeSeconds();
	EnterState(ENASoldierState::Patrol);
	GetWorldTimerManager().SetTimer(ThinkTimer, this, &ANAEnemyAIController::Think, NAAIConstants::ThinkInterval, true);
}

void ANAEnemyAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(ThinkTimer);
	StopFiring();
	Super::OnUnPossess();
}

void ANAEnemyAIController::SetDifficulty(ENADifficulty NewDifficulty)
{
	Profile = NAAI::GetDifficultyProfile(NewDifficulty);
	AppliedVisibility = 1.f;
	NextVisibilityCheck = 0.f; // Re-apply weather visibility on the next think.
	if (ANAEnemySoldier* Soldier = GetSoldier())
	{
		Soldier->Difficulty = NewDifficulty;
	}

	SightConfig->SightRadius = Profile.SightRadius;
	SightConfig->LoseSightRadius = Profile.SightRadius + 500.f;
	HearingConfig->HearingRange = Profile.HearingRange;
	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->RequestStimuliListenerUpdate();
}

ANAEnemySoldier* ANAEnemyAIController::GetSoldier() const
{
	return Cast<ANAEnemySoldier>(GetPawn());
}

ANAWeapon* ANAEnemyAIController::GetWeapon() const
{
	const ANAEnemySoldier* Soldier = GetSoldier();
	return Soldier ? Soldier->GetCurrentWeapon() : nullptr;
}

// ----- Thinking ----------------------------------------------------------------------------------

void ANAEnemyAIController::Think()
{
	ANAEnemySoldier* Soldier = GetSoldier();
	if (!Soldier || Soldier->IsDead())
	{
		StopFiring();
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float DeltaSeconds = Now - LastThinkTime;
	LastThinkTime = Now;

	// Weather and darkness change how far soldiers can see (re-checked every 2 s).
	if (Now >= NextVisibilityCheck)
	{
		NextVisibilityCheck = Now + 2.f;
		const ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this);
		const float Visibility = Environment ? Environment->GetVisibilityScale() : 1.f;
		if (FMath::Abs(Visibility - AppliedVisibility) > 0.05f)
		{
			AppliedVisibility = Visibility;
			SightConfig->SightRadius = Profile.SightRadius * Visibility;
			SightConfig->LoseSightRadius = SightConfig->SightRadius + 500.f;
			Perception->ConfigureSense(*SightConfig);
			Perception->RequestStimuliListenerUpdate();
		}
	}

	UpdatePerception(DeltaSeconds, Now);

	switch (State)
	{
	case ENASoldierState::Patrol:      TickPatrol(Now);      break;
	case ENASoldierState::Investigate: TickInvestigate(Now); break;
	case ENASoldierState::Combat:      TickCombat(Now);      break;
	case ENASoldierState::TakeCover:   TickTakeCover(Now);   break;
	case ENASoldierState::Search:      TickSearch(Now);      break;
	case ENASoldierState::Retreat:     TickRetreat(Now);     break;
	}

	const bool bFighting = State == ENASoldierState::Combat || State == ENASoldierState::TakeCover || State == ENASoldierState::Retreat;
	const bool bWary = State == ENASoldierState::Investigate || State == ENASoldierState::Search || Awareness > 0.3f;
	Soldier->SetAlert(bFighting ? ENAAlertState::Combat : bWary ? ENAAlertState::Suspicious : ENAAlertState::Unaware, Target.Get());
}

void ANAEnemyAIController::UpdatePerception(float DeltaSeconds, float Now)
{
	const APawn* Self = GetPawn();

	TArray<AActor*> Seen;
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Seen);

	ANAPlayerCharacter* Nearest = nullptr;
	float NearestDistance = TNumericLimits<float>::Max();
	for (AActor* Actor : Seen)
	{
		ANAPlayerCharacter* Player = Cast<ANAPlayerCharacter>(Actor);
		if (!Player || Player->IsDead() || Player->IsDowned())
		{
			continue; // Downed players are left alone so teammates get a chance to revive them.
		}
		if (Player->IsCloaked() && FVector::Dist(Player->GetActorLocation(), Self->GetActorLocation()) > 300.f)
		{
			continue; // A cloaked Phantom is only noticed right up close.
		}
		const float Distance = FVector::Dist(Player->GetActorLocation(), Self->GetActorLocation());
		if (Distance < NearestDistance && LineOfSightTo(Player))
		{
			Nearest = Player;
			NearestDistance = Distance;
		}
	}

	bTargetVisible = Nearest != nullptr;
	if (bTargetVisible)
	{
		Target = Nearest;
		LastKnownLocation = Nearest->GetActorLocation();
		LastSeenTime = Now;

		// Noticing is faster up close, slower far away, and slower if the player is crouched.
		float Rate = FMath::GetMappedRangeValueClamped(FVector2D(300.f, Profile.SightRadius), FVector2D(3.f, 0.5f), NearestDistance);
		if (Nearest->IsProne())
		{
			Rate *= 0.3f; // Lying flat is very hard to spot.
		}
		else if (Nearest->bIsCrouched)
		{
			Rate *= 0.6f;
		}
		Awareness = FMath::Min(1.f, Awareness + DeltaSeconds * Rate / FMath::Max(Profile.ReactionSeconds, 0.05f));
	}
	else if (State != ENASoldierState::Combat)
	{
		Awareness = FMath::Max(0.f, Awareness - DeltaSeconds * 0.15f);
	}

	if (const ANACharacterBase* Current = Cast<ANACharacterBase>(Target.Get()))
	{
		if (Current->IsDead() || Current->IsDowned())
		{
			Target = nullptr;
			bTargetVisible = false;
		}
	}
}

void ANAEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed() || UAIPerceptionSystem::GetSenseClassForStimulus(this, Stimulus) != UAISense_Hearing::StaticClass())
	{
		return; // Sight is polled every think; here we only react to noises.
	}

	const ANAPlayerCharacter* Player = Cast<ANAPlayerCharacter>(Actor);
	if (!Player || Player->IsDead())
	{
		return; // Ignore our own and allies' gunfire.
	}

	const bool bGunshot = Stimulus.Tag == FName(TEXT("Gunshot"));
	const float Distance = GetPawn() ? FVector::Dist(Stimulus.StimulusLocation, GetPawn()->GetActorLocation()) : 0.f;

	if (State == ENASoldierState::Combat || State == ENASoldierState::TakeCover || State == ENASoldierState::Retreat)
	{
		if (!bTargetVisible)
		{
			LastKnownLocation = Stimulus.StimulusLocation; // Track the fight by sound.
		}
		return;
	}

	// A nearby gunshot is alarming; distant footsteps only raise suspicion.
	Awareness = FMath::Max(Awareness, bGunshot ? (Distance < 1500.f ? 0.9f : 0.6f) : 0.4f);
	if (bGunshot || Profile.bInvestigatesSuspicion)
	{
		InvestigateLocation = Stimulus.StimulusLocation;
		EnterState(ENASoldierState::Investigate);
	}
}

void ANAEnemyAIController::ReceiveAlert(AActor* AlertedTarget, const FVector& Location)
{
	if (!AlertedTarget || State == ENASoldierState::Combat || State == ENASoldierState::TakeCover || State == ENASoldierState::Retreat)
	{
		return;
	}
	Target = AlertedTarget;
	LastKnownLocation = Location;
	LastSeenTime = GetWorld()->GetTimeSeconds();
	Awareness = 1.f;
	EnterState(ENASoldierState::Combat);
}

void ANAEnemyAIController::EnterState(ENASoldierState NewState)
{
	ANAEnemySoldier* Soldier = GetSoldier();
	const float Now = GetWorld()->GetTimeSeconds();
	State = NewState;
	StateStartTime = Now;
	WaitUntil = 0.f;
	StopMovement();

	if (!Soldier)
	{
		return;
	}

	switch (NewState)
	{
	case ENASoldierState::Patrol:
		StopFiring();
		ClearFocus(EAIFocusPriority::Gameplay);
		Soldier->SetAiming(false);
		Soldier->SetDesiredGait(ENAGait::Walk);
		if (Soldier->bIsCrouched) { Soldier->UnCrouch(); }
		break;

	case ENASoldierState::Investigate:
		StopFiring();
		Soldier->SetAiming(false);
		Soldier->SetDesiredGait(ENAGait::Run);
		MoveToPoint(InvestigateLocation, 150.f);
		break;

	case ENASoldierState::Combat:
		if (Soldier->bIsCrouched) { Soldier->UnCrouch(); }
		Soldier->SetDesiredGait(ENAGait::Run);
		Soldier->SetAiming(true);
		NextRepositionTime = Now + 0.5f;
		break;

	case ENASoldierState::TakeCover:
		StopFiring();
		Soldier->SetAiming(false);
		Soldier->SetDesiredGait(ENAGait::Run);
		MoveToPoint(CoverLocation, 60.f);
		break;

	case ENASoldierState::Search:
		StopFiring();
		Soldier->SetAiming(true);
		Soldier->SetDesiredGait(ENAGait::Walk);
		Awareness = 0.8f; // Re-spotting takes a moment, but less than the first time.
		MoveToPoint(LastKnownLocation, 150.f);
		break;

	case ENASoldierState::Retreat:
		StopFiring();
		Soldier->SetAiming(false);
		Soldier->SetDesiredGait(ENAGait::Sprint);
		MoveToPoint(CoverLocation, 60.f);
		break;
	}
}

void ANAEnemyAIController::EnterCombat()
{
	const float Now = GetWorld()->GetTimeSeconds();
	Awareness = 1.f;
	EnterState(ENASoldierState::Combat);
	NextBurstTime = Now + Profile.ReactionSeconds * 0.5f; // First shots come after a beat, not instantly.

	if (Profile.bAlertsAllies && Target.IsValid() && GetPawn())
	{
		for (TActorIterator<ANAEnemySoldier> It(GetWorld()); It; ++It)
		{
			ANAEnemySoldier* Ally = *It;
			if (Ally == GetPawn() || Ally->IsDead())
			{
				continue;
			}
			if (FVector::Dist(Ally->GetActorLocation(), GetPawn()->GetActorLocation()) <= NAAIConstants::AllyAlertRadius)
			{
				if (ANAEnemyAIController* AllyBrain = Cast<ANAEnemyAIController>(Ally->GetController()))
				{
					AllyBrain->ReceiveAlert(Target.Get(), LastKnownLocation);
				}
			}
		}
	}
}

// ----- States ------------------------------------------------------------------------------------

void ANAEnemyAIController::TickPatrol(float Now)
{
	if (Awareness >= 1.f)
	{
		EnterCombat();
		return;
	}
	if (Awareness >= 0.35f && Profile.bInvestigatesSuspicion)
	{
		InvestigateLocation = LastKnownLocation;
		EnterState(ENASoldierState::Investigate);
		return;
	}

	if (!IsMoveIdle() || Route.Num() == 0)
	{
		return;
	}

	if (WaitUntil <= 0.f)
	{
		WaitUntil = Now + FMath::FRandRange(1.f, 2.5f); // Pause at each point, look around.
	}
	else if (Now >= WaitUntil)
	{
		WaitUntil = 0.f;
		RouteIndex = (RouteIndex + 1) % Route.Num();
		MoveToPoint(Route[RouteIndex], 80.f);
	}
}

void ANAEnemyAIController::TickInvestigate(float Now)
{
	if (Awareness >= 1.f)
	{
		EnterCombat();
		return;
	}

	if (IsMoveIdle())
	{
		// Arrived (or couldn't get closer): look around the spot for a few seconds.
		const float Looking = Now - StateStartTime;
		const float Angle = Looking * 70.f;
		SetFocalPoint(InvestigateLocation + FRotator(0.f, Angle, 0.f).Vector() * 500.f + FVector(0.f, 0.f, 100.f));
		if (WaitUntil <= 0.f)
		{
			WaitUntil = Now + 3.5f;
		}
		else if (Now >= WaitUntil)
		{
			EnterState(ENASoldierState::Patrol); // Nothing found: back to the patrol route.
		}
	}
	else if (Now - StateStartTime > 20.f)
	{
		EnterState(ENASoldierState::Patrol);
	}
}

void ANAEnemyAIController::TickCombat(float Now)
{
	ANAEnemySoldier* Soldier = GetSoldier();
	ANAWeapon* Weapon = GetWeapon();
	AActor* CurrentTarget = Target.Get();
	if (!CurrentTarget)
	{
		EnterState(ENASoldierState::Search);
		return;
	}

	// Badly hurt: fall back once (Hard and Expert).
	const UNAHealthComponent* Health = Soldier->GetHealthComponent();
	if (!bHasRetreated && Profile.RetreatHealthFraction > 0.f && Health &&
		Health->GetHealth() < Health->GetMaxHealth() * Profile.RetreatHealthFraction)
	{
		const FVector ThreatEye = Cast<APawn>(CurrentTarget) ? Cast<APawn>(CurrentTarget)->GetPawnViewLocation() : LastKnownLocation;
		if (FindCover(ThreatEye, /*bRetreat*/ true, CoverLocation))
		{
			bHasRetreated = true;
			EnterState(ENASoldierState::Retreat);
			return;
		}
	}

	// Empty gun: reload, behind cover if this soldier uses cover.
	if (Weapon && (Weapon->GetAmmoInMagazine() == 0 || Weapon->IsReloading()))
	{
		StopFiring();
		if (!Weapon->IsReloading() && FMath::FRand() < Profile.CoverChance)
		{
			const FVector ThreatEye = Cast<APawn>(CurrentTarget) ? Cast<APawn>(CurrentTarget)->GetPawnViewLocation() : LastKnownLocation;
			if (FindCover(ThreatEye, false, CoverLocation))
			{
				EnterState(ENASoldierState::TakeCover);
				return;
			}
		}
		Weapon->RequestReload();
		return;
	}

	if (!bTargetVisible)
	{
		StopFiring();
		if (Now - LastSeenTime > NAAIConstants::LoseTargetSeconds)
		{
			EnterState(ENASoldierState::Search);
			return;
		}
		SetFocalPoint(LastKnownLocation + FVector(0.f, 0.f, 60.f));
		if (IsMoveIdle())
		{
			MoveToPoint(LastKnownLocation, 200.f); // Push toward where the target was.
		}
		return;
	}

	AimAtTarget();
	UpdateBurst(Now);

	// Reposition every few seconds: close distance, back off, take cover, or strafe.
	if (Now >= NextRepositionTime)
	{
		NextRepositionTime = Now + FMath::FRandRange(2.f, 4.f);
		const FVector SelfLocation = Soldier->GetActorLocation();
		const FVector ToTarget = CurrentTarget->GetActorLocation() - SelfLocation;
		const float Distance = ToTarget.Size();
		const FVector Direction = ToTarget.GetSafeNormal2D();

		if (Distance > Profile.PreferredRangeCm * 1.3f)
		{
			MoveToPoint(SelfLocation + Direction * (Distance - Profile.PreferredRangeCm), 100.f);
		}
		else if (Distance < NAAIConstants::CloseRange)
		{
			MoveToPoint(SelfLocation - Direction * 400.f, 100.f);
		}
		else if (FMath::FRand() < Profile.CoverChance * 0.5f && FindCover(Cast<APawn>(CurrentTarget)->GetPawnViewLocation(), false, CoverLocation))
		{
			EnterState(ENASoldierState::TakeCover);
		}
		else
		{
			const FVector Side = FVector::CrossProduct(Direction, FVector::UpVector) * (FMath::RandBool() ? 1.f : -1.f);
			MoveToPoint(SelfLocation + Side * FMath::FRandRange(200.f, 400.f), 60.f);
		}
	}
}

void ANAEnemyAIController::TickTakeCover(float Now)
{
	ANAEnemySoldier* Soldier = GetSoldier();
	ANAWeapon* Weapon = GetWeapon();

	if (!IsMoveIdle())
	{
		if (Now - StateStartTime > 8.f)
		{
			EnterState(ENASoldierState::Combat); // Couldn't reach cover in time.
		}
		return;
	}

	// In cover: crouch, reload, then pop back up to fight.
	if (!Soldier->bIsCrouched)
	{
		Soldier->Crouch();
	}
	if (Weapon && Weapon->GetAmmoInMagazine() < Weapon->GetStats().MagazineSize && !Weapon->IsReloading())
	{
		Weapon->RequestReload();
	}
	if (Target.IsValid())
	{
		SetFocalPoint(LastKnownLocation + FVector(0.f, 0.f, 60.f));
	}

	const bool bReady = !Weapon || (!Weapon->IsReloading() && Weapon->GetAmmoInMagazine() > 0);
	if (bReady && Now - StateStartTime > 2.5f)
	{
		EnterState(ENASoldierState::Combat);
	}
}

void ANAEnemyAIController::TickSearch(float Now)
{
	if (bTargetVisible && Awareness >= 1.f)
	{
		EnterCombat();
		return;
	}

	if (Now - StateStartTime > Profile.SearchSeconds)
	{
		Target = nullptr;
		Awareness = 0.f;
		EnterState(ENASoldierState::Patrol); // Gave up: return to patrol.
		return;
	}

	if (IsMoveIdle())
	{
		// Check random spots around where the target was last seen.
		const FVector2D Offset = FMath::RandPointInCircle(700.f);
		FVector Ground;
		if (ProjectToGround(LastKnownLocation + FVector(Offset.X, Offset.Y, 0.f), Ground))
		{
			MoveToPoint(Ground, 100.f);
			SetFocalPoint(Ground + FVector(0.f, 0.f, 100.f));
		}
	}
}

void ANAEnemyAIController::TickRetreat(float Now)
{
	ANAEnemySoldier* Soldier = GetSoldier();
	ANAWeapon* Weapon = GetWeapon();

	if (!IsMoveIdle() && Now - StateStartTime < 10.f)
	{
		return;
	}

	if (!Soldier->bIsCrouched)
	{
		Soldier->Crouch();
		Soldier->SetDesiredGait(ENAGait::Run);
	}
	if (Weapon && Weapon->GetAmmoInMagazine() < Weapon->GetStats().MagazineSize && !Weapon->IsReloading())
	{
		Weapon->RequestReload();
	}
	if (WaitUntil <= 0.f)
	{
		WaitUntil = Now + 4.f;
	}
	else if (Now >= WaitUntil)
	{
		EnterState(ENASoldierState::Combat); // Regrouped: back into the fight.
	}
}

// ----- Helpers -----------------------------------------------------------------------------------

void ANAEnemyAIController::AimAtTarget()
{
	const APawn* TargetPawn = Cast<APawn>(Target.Get());
	if (!TargetPawn)
	{
		return;
	}
	const FVector AimPoint = bAimForHead ? TargetPawn->GetPawnViewLocation() : TargetPawn->GetActorLocation() + FVector(0.f, 0.f, 20.f);
	SetFocalPoint(AimPoint + AimOffset);
}

void ANAEnemyAIController::UpdateBurst(float Now)
{
	ANAWeapon* Weapon = GetWeapon();
	const APawn* Self = GetPawn();
	const AActor* CurrentTarget = Target.Get();
	if (!Weapon || !Self || !CurrentTarget)
	{
		return;
	}

	if (bFiring)
	{
		if (Now >= BurstEndTime)
		{
			Weapon->StopFire();
			bFiring = false;
			NextBurstTime = Now + Profile.BurstPauseSeconds * FMath::FRandRange(0.7f, 1.3f);
		}
		return;
	}

	if (Now < NextBurstTime)
	{
		return;
	}

	// Only shoot once the body has turned to face the target.
	const FVector ToTarget = (CurrentTarget->GetActorLocation() - Self->GetActorLocation()).GetSafeNormal2D();
	if (FVector::DotProduct(Self->GetActorForwardVector(), ToTarget) < 0.85f)
	{
		return;
	}

	// New aim error for each burst: worse at range and against moving targets.
	const float Distance = FVector::Dist(CurrentTarget->GetActorLocation(), Self->GetActorLocation());
	float ErrorDegrees = Profile.AimErrorDegrees;
	if (CurrentTarget->GetVelocity().Size2D() > 300.f)
	{
		ErrorDegrees *= 1.4f;
	}
	AimOffset = FMath::VRand() * Distance * FMath::Tan(FMath::DegreesToRadians(ErrorDegrees)) * FMath::FRand();
	bAimForHead = FMath::FRand() < Profile.HeadshotChance;
	AimAtTarget();

	Weapon->StartFire();
	bFiring = true;
	BurstEndTime = Now + Profile.BurstSeconds * FMath::FRandRange(0.7f, 1.3f);
}

void ANAEnemyAIController::StopFiring()
{
	if (ANAWeapon* Weapon = GetWeapon())
	{
		Weapon->StopFire();
	}
	bFiring = false;
}

void ANAEnemyAIController::MoveToPoint(const FVector& Destination, float AcceptanceRadius)
{
	// Use the navmesh when the level has one; otherwise walk straight there.
	const EPathFollowingRequestResult::Type Result = MoveToLocation(Destination, AcceptanceRadius, true, true, true, true);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		MoveToLocation(Destination, AcceptanceRadius, true, false, false, true);
	}
}

bool ANAEnemyAIController::IsMoveIdle() const
{
	return GetMoveStatus() == EPathFollowingStatus::Idle;
}

bool ANAEnemyAIController::ProjectToGround(const FVector& Point, FVector& OutGround) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NAAIGround), false, GetPawn());
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Point + FVector(0.f, 0.f, 300.f), Point - FVector(0.f, 0.f, 600.f), ECC_Visibility, Params))
	{
		OutGround = Hit.ImpactPoint;
		return true;
	}
	return false;
}

bool ANAEnemyAIController::IsStandable(const FVector& Ground) const
{
	const APawn* Self = GetPawn();
	const float FeetZ = Self->GetActorLocation().Z - 90.f;
	if (FMath::Abs(Ground.Z - FeetZ) > 60.f)
	{
		return false; // Only flat ground at our level (not on top of crates).
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NAAIStandable), false, Self);
	return !GetWorld()->OverlapBlockingTestByChannel(Ground + FVector(0.f, 0.f, 95.f), FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(36.f, 88.f), Params);
}

bool ANAEnemyAIController::FindCover(const FVector& ThreatEye, bool bRetreat, FVector& OutCover) const
{
	const APawn* Self = GetPawn();
	if (!Self)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NAAICover), false, Self);
	if (const AActor* ThreatActor = Target.Get())
	{
		Params.AddIgnoredActor(ThreatActor);
	}

	const FVector SelfLocation = Self->GetActorLocation();
	const FVector FromThreat = (SelfLocation - ThreatEye).GetSafeNormal2D();
	const float Radii[] = { 350.f, 650.f, 950.f, 1300.f };
	constexpr int32 Directions = 16;

	float BestScore = TNumericLimits<float>::Max();
	bool bFound = false;

	for (const float Radius : Radii)
	{
		for (int32 Index = 0; Index < Directions; ++Index)
		{
			const FVector Dir = FRotator(0.f, 360.f * Index / Directions, 0.f).Vector();
			FVector Ground;
			if (!ProjectToGround(SelfLocation + Dir * Radius, Ground) || !IsStandable(Ground))
			{
				continue;
			}

			// Cover = the threat can't see us when crouched here.
			const FVector CrouchedEye = Ground + FVector(0.f, 0.f, 70.f);
			if (!GetWorld()->LineTraceTestByChannel(CrouchedEye, ThreatEye, ECC_Visibility, Params))
			{
				continue;
			}

			const float DistanceToThreat = FVector::Dist2D(Ground, ThreatEye);
			// Prefer close cover we can peek out of (standing sees the threat).
			const FVector StandingEye = Ground + FVector(0.f, 0.f, 150.f);
			const bool bCanPeek = !GetWorld()->LineTraceTestByChannel(StandingEye, ThreatEye, ECC_Visibility, Params);

			float Score = Radius;
			if (bRetreat)
			{
				Score -= DistanceToThreat * 0.8f; // Retreat: as far from the threat as practical.
			}
			else
			{
				Score -= bCanPeek ? 300.f : 0.f;
				if (DistanceToThreat < 500.f)
				{
					Score += 1000.f; // Too close to the enemy.
				}
				if (Profile.bFlanks)
				{
					// Expert: favour cover off to the target's side, to flank.
					const FVector Offset = (Ground - ThreatEye).GetSafeNormal2D();
					const float Sideways = 1.f - FMath::Abs(FVector::DotProduct(Offset, FromThreat));
					Score -= Sideways * 400.f;
				}
			}

			if (Score < BestScore)
			{
				BestScore = Score;
				OutCover = Ground;
				bFound = true;
			}
		}
	}
	return bFound;
}
