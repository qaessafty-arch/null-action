#include "Creatures/NACreatureAIController.h"

#include "AITypes.h"
#include "Character/NACharacterBase.h"
#include "Character/NAPlayerCharacter.h"
#include "Combat/NAHealthComponent.h"
#include "Creatures/NACreature.h"
#include "Engine/World.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "TimerManager.h"
#include "World/NAWorldEnvironment.h"

namespace NACreatureSpeeds
{
	constexpr float Roam = 220.f;
	constexpr float Stalk = 200.f;
	constexpr float Investigate = 450.f;
	constexpr float Dodge = 750.f;
	constexpr float Charge = 900.f;
	constexpr float Flee = 950.f;
}

ANACreatureAIController::ANACreatureAIController()
{
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 3000.f;
	SightConfig->LoseSightRadius = 3600.f;
	SightConfig->PeripheralVisionAngleDegrees = 80.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->SetMaxAge(6.f);

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 4000.f; // Keen hearing: gunfire carries far.
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->SetMaxAge(4.f);

	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*Perception);
}

ANACreature* ANACreatureAIController::GetCreature() const
{
	return Cast<ANACreature>(GetPawn());
}

void ANACreatureAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ANACreatureAIController::OnTargetPerceptionUpdated);
	LastThinkTime = GetWorld()->GetTimeSeconds();
	OrbitDirection = FMath::RandBool() ? 1.f : -1.f;
	EnterState(ENACreatureState::Roam);
	GetWorldTimerManager().SetTimer(ThinkTimer, this, &ANACreatureAIController::Think, 0.1f, true);
}

void ANACreatureAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(ThinkTimer);
	Super::OnUnPossess();
}

void ANACreatureAIController::Think()
{
	ANACreature* Creature = GetCreature();
	if (!Creature || Creature->IsDead())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float DeltaSeconds = Now - LastThinkTime;
	LastThinkTime = Now;

	UpdatePrey(Now);

	// Shot by someone it didn't see: turn on the shooter (unless already fleeing).
	if (!Prey.IsValid() && State != ENACreatureState::Flee)
	{
		if (AActor* Attacker = Creature->GetLastAttacker())
		{
			Creature->ClearLastAttacker();
			const ANACharacterBase* AttackerBody = Cast<ANACharacterBase>(Attacker);
			if (AttackerBody && !AttackerBody->IsDead())
			{
				Prey = Attacker;
				LastKnownLocation = Attacker->GetActorLocation();
				LastSeenTime = Now;
				EnterState(ENACreatureState::Hunt);
			}
		}
	}

	// Badly hurt: run home once and recover.
	const UNAHealthComponent* Health = Creature->GetHealthComponent();
	if (!bHasFled && State != ENACreatureState::Flee && Health && Health->GetHealth() < Health->GetMaxHealth() * FleeHealthFraction)
	{
		bHasFled = true;
		EnterState(ENACreatureState::Flee);
	}

	// Prey left the territory: let it go.
	if (Prey.IsValid() && State != ENACreatureState::Flee && State != ENACreatureState::Return &&
		FVector::Dist(Prey->GetActorLocation(), Creature->GetDenLocation()) > TerritoryRadius)
	{
		Prey = nullptr;
		EnterState(ENACreatureState::Return);
	}

	switch (State)
	{
	case ENACreatureState::Roam:   TickRoam(Now);                 break;
	case ENACreatureState::Stalk:  TickStalk(Now, DeltaSeconds);  break;
	case ENACreatureState::Hunt:   TickHunt(Now);                 break;
	case ENACreatureState::Flee:   TickFlee(Now, DeltaSeconds);   break;
	case ENACreatureState::Return: TickReturn(Now);               break;
	}

	ENACreatureMood Mood = ENACreatureMood::Roaming;
	switch (State)
	{
	case ENACreatureState::Stalk: Mood = ENACreatureMood::Stalking; break;
	case ENACreatureState::Hunt:  Mood = ENACreatureMood::Hunting;  break;
	case ENACreatureState::Flee:  Mood = ENACreatureMood::Fleeing;  break;
	default: break;
	}
	Creature->SetMood(Mood, Prey.Get());
}

void ANACreatureAIController::UpdatePrey(float Now)
{
	const APawn* Self = GetPawn();
	TArray<AActor*> Seen;
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Seen);

	AActor* Nearest = nullptr;
	float NearestDistance = TNumericLimits<float>::Max();
	for (AActor* Candidate : Seen)
	{
		const ANAPlayerCharacter* Player = Cast<ANAPlayerCharacter>(Candidate);
		if (!Player || Player->IsDead())
		{
			continue;
		}
		if (Player->IsCloaked() && GetPawn() && FVector::Dist(Player->GetActorLocation(), GetPawn()->GetActorLocation()) > 300.f)
		{
			continue; // A cloaked Phantom is only noticed right up close.
		}
		const float Distance = FVector::Dist(Player->GetActorLocation(), Self->GetActorLocation());
		if (Distance < NearestDistance && LineOfSightTo(Player))
		{
			Nearest = Candidate;
			NearestDistance = Distance;
		}
	}

	bPreyVisible = Nearest != nullptr;
	if (bPreyVisible)
	{
		if (!Prey.IsValid() || State == ENACreatureState::Roam || State == ENACreatureState::Return)
		{
			Prey = Nearest;
		}
		if (Prey.Get() == Nearest)
		{
			LastKnownLocation = Nearest->GetActorLocation();
			LastSeenTime = Now;
		}
	}

	if (const ANACharacterBase* Current = Cast<ANACharacterBase>(Prey.Get()))
	{
		if (Current->IsDead())
		{
			Prey = nullptr; // Killed its prey: back to roaming.
			bPreyVisible = false;
			if (State == ENACreatureState::Hunt || State == ENACreatureState::Stalk)
			{
				EnterState(ENACreatureState::Return);
			}
		}
	}
}

void ANACreatureAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed() || UAIPerceptionSystem::GetSenseClassForStimulus(this, Stimulus) != UAISense_Hearing::StaticClass())
	{
		return;
	}
	const ANAPlayerCharacter* Player = Cast<ANAPlayerCharacter>(Actor);
	if (!Player || Player->IsDead())
	{
		return;
	}

	// Drawn to noise: gunshots especially. Comes in to stalk the source.
	if (State == ENACreatureState::Roam || State == ENACreatureState::Return)
	{
		Prey = Actor;
		LastKnownLocation = Stimulus.StimulusLocation;
		LastSeenTime = GetWorld()->GetTimeSeconds();
		EnterState(ENACreatureState::Stalk);
	}
	else if (!bPreyVisible && Prey.Get() == Actor)
	{
		LastKnownLocation = Stimulus.StimulusLocation;
	}
}

void ANACreatureAIController::EnterState(ENACreatureState NewState)
{
	State = NewState;
	StateStartTime = GetWorld()->GetTimeSeconds();
	WaitUntil = 0.f;
	RecoilUntil = 0.f;
	StopMovement();

	ANACreature* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}

	switch (NewState)
	{
	case ENACreatureState::Roam:   Creature->SetMoveSpeed(NACreatureSpeeds::Roam);   ClearFocus(EAIFocusPriority::Gameplay); break;
	case ENACreatureState::Stalk:  Creature->SetMoveSpeed(bPreyVisible ? NACreatureSpeeds::Stalk : NACreatureSpeeds::Investigate); break;
	case ENACreatureState::Hunt:
	{
		// Night predator: faster charges in the dark.
		const ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this);
		Creature->SetMoveSpeed(NACreatureSpeeds::Charge * (Environment && Environment->IsNight() ? 1.2f : 1.f));
		break;
	}
	case ENACreatureState::Flee:   Creature->SetMoveSpeed(NACreatureSpeeds::Flee);   ClearFocus(EAIFocusPriority::Gameplay); MoveToPoint(Creature->GetDenLocation(), 150.f); break;
	case ENACreatureState::Return: Creature->SetMoveSpeed(NACreatureSpeeds::Roam);   ClearFocus(EAIFocusPriority::Gameplay); MoveToPoint(Creature->GetDenLocation(), 300.f); break;
	}
}

void ANACreatureAIController::TickRoam(float Now)
{
	ANACreature* Creature = GetCreature();
	if (bPreyVisible && Prey.IsValid())
	{
		// Close and unaware prey gets ambushed straight away; otherwise stalk it first.
		const float Distance = FVector::Dist(Prey->GetActorLocation(), Creature->GetActorLocation());
		EnterState(Distance < 900.f && !IsPreyLookingAtMe(0.3f) ? ENACreatureState::Hunt : ENACreatureState::Stalk);
		return;
	}

	if (!IsMoveIdle())
	{
		return;
	}
	if (WaitUntil <= 0.f)
	{
		WaitUntil = Now + FMath::FRandRange(2.f, 5.f); // Pause, sniff around.
	}
	else if (Now >= WaitUntil)
	{
		WaitUntil = 0.f;
		const FVector2D Offset = FMath::RandPointInCircle(RoamRadius);
		MoveToPoint(Creature->GetDenLocation() + FVector(Offset.X, Offset.Y, 0.f), 100.f);
	}
}

void ANACreatureAIController::TickStalk(float Now, float DeltaSeconds)
{
	ANACreature* Creature = GetCreature();
	AActor* Target = Prey.Get();
	if (!Target)
	{
		EnterState(ENACreatureState::Return);
		return;
	}

	if (!bPreyVisible)
	{
		// Heard but not seen: move in on the sound.
		Creature->SetMoveSpeed(NACreatureSpeeds::Investigate);
		if (IsMoveIdle())
		{
			MoveToPoint(LastKnownLocation, 300.f);
		}
		if (Now - LastSeenTime > 8.f)
		{
			Prey = nullptr;
			EnterState(ENACreatureState::Return);
		}
		return;
	}

	SetFocus(Target);
	const float Stalked = Now - StateStartTime;

	// Opening: prey looks away, or patience runs out (much sooner at night).
	const ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this);
	const float Patience = Environment && Environment->IsNight() ? 3.5f : 7.f;
	if ((Stalked > 1.5f && !IsPreyLookingAtMe(0.3f)) || Stalked > Patience)
	{
		EnterState(ENACreatureState::Hunt);
		return;
	}

	// Being aimed at: dart sideways.
	const ANACharacterBase* TargetBody = Cast<ANACharacterBase>(Target);
	if (TargetBody && TargetBody->IsAiming() && IsPreyLookingAtMe(0.97f) && Now >= NextDodgeTime)
	{
		NextDodgeTime = Now + 1.5f;
		OrbitDirection *= -1.f;
		const FVector Away = (Creature->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
		const FVector Side = FVector::CrossProduct(Away, FVector::UpVector) * OrbitDirection;
		Creature->SetMoveSpeed(NACreatureSpeeds::Dodge);
		MoveToPoint(Creature->GetActorLocation() + Side * 450.f, 50.f);
		return;
	}

	// Circle at a distance, low and slow.
	Creature->SetMoveSpeed(NACreatureSpeeds::Stalk);
	OrbitAngle += DeltaSeconds * 18.f * OrbitDirection;
	const FVector FromPrey = (Creature->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
	const FVector OrbitDir = FromPrey.RotateAngleAxis(25.f * OrbitDirection, FVector::UpVector);
	MoveToPoint(Target->GetActorLocation() + OrbitDir * StalkDistance, 80.f);
}

void ANACreatureAIController::TickHunt(float Now)
{
	ANACreature* Creature = GetCreature();
	AActor* Target = Prey.Get();
	if (!Target)
	{
		EnterState(ENACreatureState::Return);
		return;
	}

	if (!bPreyVisible && Now - LastSeenTime > 4.f)
	{
		EnterState(ENACreatureState::Stalk); // Lost it: go back to hunting by ear.
		return;
	}

	SetFocus(Target);

	// After a bite, spring back before the next charge (hit-and-run).
	if (Now < RecoilUntil)
	{
		return;
	}

	const float Distance = FVector::Dist(Target->GetActorLocation(), Creature->GetActorLocation());
	if (Distance <= Creature->GetBiteRange() && Creature->TryBite(Target))
	{
		const FVector Back = (Creature->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
		MoveToPoint(Creature->GetActorLocation() + Back * 350.f, 50.f);
		RecoilUntil = Now + 0.8f;
		return;
	}

	const ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this);
	Creature->SetMoveSpeed(NACreatureSpeeds::Charge * (Environment && Environment->IsNight() ? 1.2f : 1.f));
	MoveToPoint(bPreyVisible ? Target->GetActorLocation() : LastKnownLocation, Creature->GetBiteRange() * 0.6f);
}

void ANACreatureAIController::TickFlee(float Now, float DeltaSeconds)
{
	ANACreature* Creature = GetCreature();
	if (!IsMoveIdle() && Now - StateStartTime < 15.f)
	{
		return;
	}

	// At the den: lick wounds until recovered, then roam again.
	if (UNAHealthComponent* Health = Creature->GetHealthComponent())
	{
		Health->Heal(DenHealPerSecond * DeltaSeconds);
		if (Health->GetHealth() >= Health->GetMaxHealth() * RecoverHealthFraction)
		{
			Prey = nullptr;
			bHasFled = false;
			EnterState(ENACreatureState::Roam);
		}
	}
}

void ANACreatureAIController::TickReturn(float Now)
{
	if (bPreyVisible && Prey.IsValid())
	{
		EnterState(ENACreatureState::Stalk);
		return;
	}
	if (IsMoveIdle() || Now - StateStartTime > 20.f)
	{
		EnterState(ENACreatureState::Roam);
	}
}

void ANACreatureAIController::MoveToPoint(const FVector& Destination, float AcceptanceRadius)
{
	const EPathFollowingRequestResult::Type Result = MoveToLocation(Destination, AcceptanceRadius, true, true, true, false);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		MoveToLocation(Destination, AcceptanceRadius, true, false, false, false);
	}
}

bool ANACreatureAIController::IsMoveIdle() const
{
	return GetMoveStatus() == EPathFollowingStatus::Idle;
}

bool ANACreatureAIController::IsPreyLookingAtMe(float MinDot) const
{
	const APawn* PreyPawn = Cast<APawn>(Prey.Get());
	const APawn* Self = GetPawn();
	if (!PreyPawn || !Self)
	{
		return false;
	}
	const FVector Look = PreyPawn->GetBaseAimRotation().Vector().GetSafeNormal2D();
	const FVector ToMe = (Self->GetActorLocation() - PreyPawn->GetActorLocation()).GetSafeNormal2D();
	return FVector::DotProduct(Look, ToMe) >= MinDot;
}
