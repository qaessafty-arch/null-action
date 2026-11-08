// World events: the world generates activities on its own every 8-12 minutes (server-run, shown to everyone).
#include "Core/NAGameMode.h"

#include "NullAction.h"
#include "Character/NAPlayerCharacter.h"
#include "Combat/NAHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/NAGameState.h"
#include "Creatures/NACreature.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Interaction/NALootContainer.h"
#include "Interaction/NAResourceNode.h"
#include "Inventory/NAItems.h"
#include "TimerManager.h"
#include "World/NAEventActors.h"
#include "World/NATerrainManager.h"
#include "World/NAWorldEnvironment.h"

namespace NAEvents
{
	constexpr float EventSeconds = 360.f;
	constexpr float StormSeconds = 75.f;
	constexpr int32 RewardXP = 150;
}

float ANAGameMode::ServerTimeForEvents() const
{
	const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return State ? static_cast<float>(State->GetServerWorldTimeSeconds()) : 0.f;
}

void ANAGameMode::ScheduleNextWorldEvent(float DelaySeconds)
{
	GetWorldTimerManager().SetTimer(WorldEventTimer, this, &ANAGameMode::StartRandomWorldEvent, DelaySeconds, false);
}

void ANAGameMode::StartRandomWorldEvent()
{
	if (!StartWorldEvent(FString()))
	{
		ScheduleNextWorldEvent(60.f); // Busy; try again shortly.
	}
}

bool ANAGameMode::FindEventSpot(FVector& OutLocation) const
{
	// Open, dry ground 40-90 m from the arena, away from the lake, crater and cabin.
	for (int32 Attempt = 0; Attempt < 30; ++Attempt)
	{
		const FVector2D Dir = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal();
		const FVector2D Spot = Dir * FMath::FRandRange(4000.f, 9000.f);
		const float Height = NATerrain::HeightAt(Spot.X, Spot.Y);
		if (Height < NATerrain::WaterLevel() + 100.f || Height > 2500.f || NATerrain::NormalAt(Spot.X, Spot.Y).Z < 0.85f ||
			FVector2D::Distance(Spot, NATerrain::CraterCenter()) < 4000.f || FVector2D::Distance(Spot, NATerrain::CabinCenter()) < 1500.f ||
			FMath::Max(FMath::Abs(Spot.X), FMath::Abs(Spot.Y)) < 4500.f)
		{
			continue;
		}
		OutLocation = FVector(Spot.X, Spot.Y, Height);
		return true;
	}
	return false;
}

bool ANAGameMode::StartWorldEvent(const FString& Which)
{
	ANAGameState* State = GetGameState<ANAGameState>();
	if (!State || CurrentEvent != EWorldEvent::None)
	{
		return false;
	}
	EWorldEvent Pick = EWorldEvent::None;
	if (Which.Equals(TEXT("sighting"), ESearchCase::IgnoreCase))     { Pick = EWorldEvent::Sighting; }
	else if (Which.Equals(TEXT("supply"), ESearchCase::IgnoreCase))  { Pick = EWorldEvent::Supply; }
	else if (Which.Equals(TEXT("beacon"), ESearchCase::IgnoreCase))  { Pick = EWorldEvent::Beacon; }
	else if (Which.Equals(TEXT("storm"), ESearchCase::IgnoreCase))   { Pick = EWorldEvent::Storm; }
	else if (Which.IsEmpty())                                        { Pick = static_cast<EWorldEvent>(FMath::RandRange(1, 4)); }
	else                                                             { return false; }

	FVector Spot = FVector::ZeroVector;
	if (Pick != EWorldEvent::Storm && !FindEventSpot(Spot))
	{
		return false;
	}
	UWorld* World = GetWorld();
	const float Now = ServerTimeForEvents();
	EventEndsAt = Now + (Pick == EWorldEvent::Storm ? NAEvents::StormSeconds : NAEvents::EventSeconds);

	switch (Pick)
	{
	case EWorldEvent::Sighting:
	{
		// An alpha Ash Stalker: bigger, tougher, no respawn. Kill it for the reward.
		const FTransform Where(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Spot + FVector(0.f, 0.f, 120.f));
		ANACreature* Alpha = World->SpawnActorDeferred<ANACreature>(ANACreature::StaticClass(), Where, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Alpha)
		{
			return false;
		}
		Alpha->SetSpawnGroup(2);
		Alpha->SetDenLocation(Spot);
		Alpha->GetCapsuleComponent()->SetRelativeScale3D(FVector(1.3f));
		Alpha->FinishSpawning(Where);
		if (UNAHealthComponent* AlphaHealth = Alpha->GetHealthComponent())
		{
			AlphaHealth->SetMaxHealth(450.f);
		}
		EventActor = Alpha;
		State->SetEvent(TEXT("CREATURE SIGHTING"), TEXT("An alpha Ash Stalker was seen. Hunt it down."), Spot, EventEndsAt);
		break;
	}
	case EWorldEvent::Supply:
	{
		const FTransform Where(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Spot + FVector(0.f, 0.f, 5.f));
		ANALootContainer* Crate = World->SpawnActorDeferred<ANALootContainer>(ANALootContainer::StaticClass(), Where, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Crate)
		{
			return false;
		}
		Crate->bHighQuality = true;
		Crate->bEventCrate = true;
		Crate->FinishSpawning(Where);
		EventActor = Crate;
		State->SetEvent(TEXT("SUPPLY DROP"), TEXT("A supply crate came down. First to open it gets the rare loot."), Spot, EventEndsAt);
		break;
	}
	case EWorldEvent::Beacon:
	{
		const FTransform Where(FRotator::ZeroRotator, Spot);
		ANADistressBeacon* Beacon = World->SpawnActor<ANADistressBeacon>(ANADistressBeacon::StaticClass(), Where);
		if (!Beacon)
		{
			return false;
		}
		EventActor = Beacon;
		State->SetEvent(TEXT("DISTRESS SIGNAL"), TEXT("Reach the beacon, switch it on, and hold the area."), Spot, EventEndsAt);
		break;
	}
	case EWorldEvent::Storm:
	{
		if (ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this))
		{
			Environment->SetWeather(ENAWeather::Thunderstorm, 10.f);
		}
		NextStormStrike = Now + 6.f;
		State->SetEvent(TEXT("EMBER STORM"), TEXT("Burning embers are falling. Watch for red circles and keep moving."), FVector::ZeroVector, EventEndsAt);
		break;
	}
	default:
		return false;
	}

	CurrentEvent = Pick;
	BroadcastSystemMessage(FString::Printf(TEXT("WORLD EVENT - %s: %s"), *State->GetEventTitle(), *State->GetEventDetail()));
	GetWorldTimerManager().SetTimer(WorldEventTick, this, &ANAGameMode::TickWorldEvent, 0.5f, true);
	return true;
}

void ANAGameMode::TickWorldEvent()
{
	ANAGameState* State = GetGameState<ANAGameState>();
	const float Now = ServerTimeForEvents();
	if (CurrentEvent == EWorldEvent::None || !State)
	{
		GetWorldTimerManager().ClearTimer(WorldEventTick);
		return;
	}

	if (CurrentEvent == EWorldEvent::Sighting)
	{
		if (AActor* Alpha = EventActor.Get())
		{
			State->SetEventLocation(Alpha->GetActorLocation()); // The marker follows the creature.
		}
		else
		{
			EndWorldEvent(false, nullptr, TEXT("The alpha vanished."));
			return;
		}
	}
	else if (CurrentEvent == EWorldEvent::Beacon)
	{
		if (const ANADistressBeacon* Beacon = Cast<ANADistressBeacon>(EventActor.Get()))
		{
			if (Beacon->IsActive())
			{
				State->SetEventDetail(FString::Printf(TEXT("Transmitting... hold the area for %d s."), FMath::CeilToInt(Beacon->GetTransmitRemaining())));
				EventEndsAt = FMath::Max(EventEndsAt, Now + Beacon->GetTransmitRemaining() + 5.f); // Don't time out mid-transmission.
			}
		}
	}
	else if (CurrentEvent == EWorldEvent::Storm && Now >= NextStormStrike)
	{
		// Embers fall around random players outside (a red circle gives 1.6 s to move).
		NextStormStrike = Now + FMath::FRandRange(1.f, 2.f);
		TArray<ANAPlayerCharacter*> Targets;
		for (TActorIterator<ANAPlayerCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->IsDead()) { Targets.Add(*It); }
		}
		if (Targets.Num() > 0)
		{
			const ANAPlayerCharacter* Target = Targets[FMath::RandRange(0, Targets.Num() - 1)];
			const FVector2D Offset = FMath::RandPointInCircle(900.f);
			FVector Ground = Target->GetActorLocation() + FVector(Offset.X, Offset.Y, 0.f);
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(NAEmberStrike), false);
			if (GetWorld()->LineTraceSingleByChannel(Hit, Ground + FVector(0.f, 0.f, 3000.f), Ground - FVector(0.f, 0.f, 3000.f), ECC_Visibility, Params))
			{
				Ground = Hit.ImpactPoint;
			}
			GetWorld()->SpawnActor<ANAEmberStrike>(ANAEmberStrike::StaticClass(), FTransform(Ground));
		}
	}

	if (Now >= EventEndsAt)
	{
		if (CurrentEvent == EWorldEvent::Storm)
		{
			// Everyone who lived through it earns a little.
			for (TActorIterator<ANAPlayerCharacter> It(GetWorld()); It; ++It)
			{
				if (!It->IsDead()) { AwardXP(It->GetController(), NAEvents::RewardXP / 2, TEXT("Survived the Ember Storm")); }
			}
			if (ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this))
			{
				Environment->SetWeather(ENAWeather::Cloudy, 30.f);
			}
			EndWorldEvent(true, nullptr, TEXT("The Ember Storm has passed."));
		}
		else
		{
			EndWorldEvent(false, nullptr, TEXT("Nobody answered in time."));
		}
	}
}

void ANAGameMode::EndWorldEvent(bool bSuccess, AController* Winner, const FString& Outcome)
{
	ANAGameState* State = GetGameState<ANAGameState>();
	const FString Title = State ? State->GetEventTitle() : FString();
	if (bSuccess && Winner)
	{
		AwardXP(Winner, NAEvents::RewardXP, Title);
	}
	BroadcastSystemMessage(FString::Printf(TEXT("WORLD EVENT %s - %s"), bSuccess ? TEXT("COMPLETE") : TEXT("ENDED"), *Outcome));

	if (AActor* Leftover = EventActor.Get())
	{
		if (!bSuccess && (Leftover->IsA<ANALootContainer>() || Leftover->IsA<ANADistressBeacon>()))
		{
			Leftover->Destroy();
		}
		else if (ANACreature* Alpha = Cast<ANACreature>(Leftover))
		{
			if (!Alpha->IsDead()) { Alpha->Destroy(); }
		}
	}
	EventActor = nullptr;
	CurrentEvent = EWorldEvent::None;
	if (State)
	{
		State->ClearEvent();
	}
	GetWorldTimerManager().ClearTimer(WorldEventTick);
	ScheduleNextWorldEvent(FMath::FRandRange(480.f, 720.f));
}

void ANAGameMode::OnEventCrateLooted(ANALootContainer* Crate, AController* Looter)
{
	if (CurrentEvent == EWorldEvent::Supply && Crate == EventActor.Get())
	{
		EndWorldEvent(true, Looter, FString::Printf(TEXT("%s claimed the supply drop."), *DisplayNameOf(nullptr, Looter)));
	}
}

void ANAGameMode::OnBeaconFinished(ANADistressBeacon* Beacon, AController* Holder)
{
	if (CurrentEvent != EWorldEvent::Beacon || Beacon != EventActor.Get())
	{
		return;
	}
	if (Holder)
	{
		// Rescuers leave supplies at the beacon.
		const FName Reward = NAItems::RandomItemOfRarity(FMath::FRand() < 0.3f ? ENAItemRarity::Epic : ENAItemRarity::Rare);
		if (Reward != NAME_None)
		{
			ANAItemPickup::Drop(GetWorld(), Beacon->GetActorLocation() + FVector(150.f, 0.f, 30.f), Reward, 1);
		}
		EndWorldEvent(true, Holder, TEXT("The beacon got through. Supplies were dropped at the beacon."));
	}
	else
	{
		EndWorldEvent(false, nullptr, TEXT("Nobody held the beacon area."));
	}
}

void ANAGameMode::OnEventCreatureKilled(ANACreature* Creature, AController* Killer)
{
	if (CurrentEvent != EWorldEvent::Sighting || Creature != EventActor.Get())
	{
		return;
	}
	const FName Reward = NAItems::RandomItemOfRarity(FMath::FRand() < 0.35f ? ENAItemRarity::Epic : ENAItemRarity::Rare);
	if (Reward != NAME_None)
	{
		ANAItemPickup::Drop(GetWorld(), Creature->GetActorLocation() + FVector(0.f, 180.f, 30.f), Reward, 1);
	}
	EndWorldEvent(Killer != nullptr, Killer, FString::Printf(TEXT("%s brought down the alpha."), *DisplayNameOf(nullptr, Killer)));
}

void ANAGameMode::SpawnShipwreck()
{
	// On the floor of Mirror Lake, ~6 m under the surface; a high-quality crate inside (refills slowly).
	const FVector2D Spot(NATerrain::LakeCenter().X + 1500.f, NATerrain::LakeCenter().Y - 1500.f);
	const FTransform Where(FRotator(0.f, 35.f, 0.f), FVector(Spot.X, Spot.Y, NATerrain::HeightAt(Spot.X, Spot.Y)));
	if (ANAShipwreck* Wreck = GetWorld()->SpawnActor<ANAShipwreck>(ANAShipwreck::StaticClass(), Where))
	{
		const FTransform CrateAt = Wreck->CrateTransform();
		if (ANALootContainer* Crate = GetWorld()->SpawnActorDeferred<ANALootContainer>(ANALootContainer::StaticClass(), CrateAt, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Crate->bHighQuality = true;
			Crate->FinishSpawning(CrateAt);
		}
	}
}
