// HUD pieces for Milestone 5B: downed/revive/carry, Ash Rebirth info and class powers.
#include "Core/NAHUD.h"

#include "Abilities/NAAbilityComponent.h"
#include "Character/NAPlayerCharacter.h"
#include "Combat/NAHealthComponent.h"
#include "Core/NAPlayerState.h"
#include "Creatures/NACreature.h"
#include "Creatures/NABoss.h"
#include "Creatures/NAHorse.h"
#include "Creatures/NASkyMount.h"
#include "Creatures/NADeer.h"
#include "Creatures/NAWolf.h"
#include "Core/NAGameState.h"
#include "Core/NAPlayerController.h"
#include "Survival/NASurvivalComponent.h"
#include "Hunting/NACreatureTrack.h"
#include "AI/NAEnemySoldier.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FLinearColor PowerAccent(0.95f, 0.22f, 0.16f);
	const FLinearColor PowerText(0.92f, 0.92f, 0.92f);
	const FLinearColor PowerDim(0.f, 0.f, 0.f, 0.55f);
	const FLinearColor AshColor(1.f, 0.5f, 0.15f);

	float HudServerTime(const UWorld* World)
	{
		const AGameStateBase* State = World ? World->GetGameState() : nullptr;
		return State ? static_cast<float>(State->GetServerWorldTimeSeconds()) : 0.f;
	}
}

bool ANAHUD::ProjectToScreen(const FVector& WorldLocation, FVector2D& OutScreen) const
{
	const APlayerController* PC = GetOwningPlayerController();
	if (!PC || !Canvas)
	{
		return false;
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	if (FVector::DotProduct(ViewRotation.Vector(), WorldLocation - ViewLocation) <= 0.f)
	{
		return false; // Behind the camera.
	}
	const FVector Screen = Canvas->Project(WorldLocation);
	OutScreen = FVector2D(Screen.X, Screen.Y);
	return true;
}

void ANAHUD::DrawDeathScreen(float ScreenW, float ScreenH)
{
	const APlayerController* PC = GetOwningPlayerController();
	const ANAPlayerState* State = PC ? PC->GetPlayerState<ANAPlayerState>() : nullptr;
	const float CenterX = ScreenW * 0.5f;
	const float CenterY = ScreenH * 0.5f;

	const bool bRebirth = State && HudServerTime(GetWorld()) - State->LastRebirthTime < 10.f;
	DrawRect(bRebirth ? FLinearColor(0.2f, 0.08f, 0.f, 0.4f) : FLinearColor(0.15f, 0.f, 0.f, 0.35f), 0.f, 0.f, ScreenW, ScreenH);
	DrawText(TEXT("YOU WERE KILLED"), PowerAccent, CenterX - 110.f, CenterY - 30.f, nullptr, 2.f);
	if (bRebirth)
	{
		DrawText(FString::Printf(TEXT("ASH REBIRTH - rising at %s with your items"), *State->GetAttunedShrineName()),
			AshColor, CenterX - 190.f, CenterY + 15.f, nullptr, 1.1f);
		DrawText(FString::Printf(TEXT("Ash charges left: %d"), State->GetAshCharges()), PowerText, CenterX - 70.f, CenterY + 38.f, nullptr, 1.f);
	}
	else
	{
		DrawText(TEXT("Respawning...   Your items were dropped where you fell."), PowerText, CenterX - 200.f, CenterY + 15.f, nullptr, 1.1f);
	}
}

void ANAHUD::DrawMounted(ANAHorse* Horse, float ScreenW, float ScreenH)
{
	const float CenterX = ScreenW * 0.5f;
	const ANAPlayerCharacter* Rider = Horse->GetRider();

	// Rider vitals (bottom-left).
	const float BarX = 28.f;
	float BarY = ScreenH - 96.f;
	DrawPanel(BarX - 12.f, BarY - 10.f, 350.f, ScreenH - BarY - 4.f);
	if (const UNAHealthComponent* RiderHealth = Rider ? Rider->GetHealthComponent() : nullptr)
	{
		DrawBar(BarX, BarY, 220.f, 18.f, RiderHealth->GetHealth() / FMath::Max(RiderHealth->GetMaxHealth(), 1.f),
			FLinearColor(0.8f, 0.12f, 0.1f), FString::Printf(TEXT("HEALTH %.0f"), RiderHealth->GetHealth()));
		BarY += 24.f;
	}
	if (const UNASurvivalComponent* Survival = Rider ? Rider->GetSurvivalComponent() : nullptr)
	{
		DrawBar(BarX, BarY, 110.f, 10.f, Survival->GetHungerFraction(), FLinearColor(0.85f, 0.45f, 0.1f), TEXT("FOOD"));
		BarY += 16.f;
		DrawBar(BarX, BarY, 110.f, 10.f, Survival->GetThirstFraction(), FLinearColor(0.15f, 0.65f, 0.85f), TEXT("WATER"));
	}

	// Horse panel (bottom centre).
	const float PanelX = CenterX - 170.f;
	const float PanelY = ScreenH - 118.f;
	DrawPanel(PanelX - 10.f, PanelY - 8.f, 360.f, 92.f);
	const ANASkyMount* Glider = Cast<ANASkyMount>(Horse);
	if (Glider)
	{
		DrawText(FString::Printf(TEXT("ASH GLIDER  -  %s   ALT %.0f m"), Glider->IsFlying() ? (Horse->IsGalloping() ? TEXT("BOOST") : TEXT("FLYING")) : TEXT("LANDED"),
			Glider->GetAltitudeAboveGround() / 100.f), AshColor, PanelX, PanelY, nullptr, 1.f);
		if (Glider->IsInStorm() && Glider->IsFlying())
		{
			DrawText(TEXT("!! STORM - strong gusts, dangerous flying"), PowerAccent, CenterX - 170.f, ScreenH - 150.f, nullptr, 1.1f);
		}
	}
	else
	{
		DrawText(Horse->IsGalloping() ? TEXT("HORSE  -  GALLOP") : TEXT("HORSE"), AshColor, PanelX, PanelY, nullptr, 1.f);
	}
	if (const UNAHealthComponent* HorseHealth = Horse->GetHealthComponent())
	{
		DrawBar(PanelX, PanelY + 20.f, 340.f, 14.f, HorseHealth->GetHealth() / FMath::Max(HorseHealth->GetMaxHealth(), 1.f),
			FLinearColor(0.7f, 0.2f, 0.12f), FString::Printf(TEXT("HEALTH %.0f"), HorseHealth->GetHealth()));
	}
	const float HorseStamina = Horse->GetStaminaFraction();
	DrawBar(PanelX, PanelY + 40.f, 340.f, 14.f, HorseStamina, HorseStamina < 0.2f ? FLinearColor(0.6f, 0.45f, 0.1f) : FLinearColor(0.95f, 0.8f, 0.2f),
		HorseStamina < 0.05f ? TEXT("WINDED") : TEXT("STAMINA"));
	DrawText(Glider ? (Glider->IsFlying() ? TEXT("W fly where you look   Space climb   C dive   Shift boost   E land (near ground)")
		: TEXT("Space take off   WASD walk   E dismount")) : TEXT("WASD ride   Shift gallop   Space jump   E dismount"),
		PowerText, PanelX, PanelY + 62.f, nullptr, 0.8f);
	DrawText(FString::Printf(TEXT("%.0f km/h"), Horse->GetVelocity().Size2D() * 0.036f), PowerText, ScreenW - 140.f, ScreenH - 60.f, nullptr, 1.2f);
}

void ANAHUD::DrawProgressAndEvents(ANAPlayerCharacter* Player, float ScreenW, float ScreenH)
{
	UWorld* World = GetWorld();
	const float CenterX = ScreenW * 0.5f;
	const APlayerController* PC = GetOwningPlayerController();
	const ANAPlayerController* NAPC = Cast<ANAPlayerController>(PC);
	const ANAPlayerState* State = PC ? PC->GetPlayerState<ANAPlayerState>() : nullptr;

	// ----- XP bar along the bottom edge, level badge, "+XP" pop-ups -----
	if (State)
	{
		const float Need = static_cast<float>(ANAPlayerState::XPForNextLevel(State->GetPlayerLevel()));
		const float Fraction = State->GetPlayerLevel() >= ANAPlayerState::MaxLevel ? 1.f : State->GetXP() / FMath::Max(1.f, Need);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), 0.f, ScreenH - 5.f, ScreenW, 5.f);
		DrawRect(AshColor, 0.f, ScreenH - 5.f, ScreenW * Fraction, 5.f);
		FString Badge = FString::Printf(TEXT("LV %d"), State->GetPlayerLevel());
		if (State->GetSkillPoints() > 0)
		{
			Badge += FString::Printf(TEXT("   %d skill point%s - press K"), State->GetSkillPoints(), State->GetSkillPoints() == 1 ? TEXT("") : TEXT("s"));
		}
		DrawText(Badge, AshColor, 22.f, ScreenH - 26.f, nullptr, 0.85f);
	}
	if (NAPC && World)
	{
		const float Since = World->GetTimeSeconds() - NAPC->GetLastXPTime();
		if (Since < 2.5f)
		{
			const float Alpha = 1.f - Since / 2.5f;
			DrawText(FString::Printf(TEXT("+%d XP  %s"), NAPC->GetLastXPAmount(), *NAPC->GetLastXPReason()),
				FLinearColor(1.f, 0.7f, 0.25f, Alpha), CenterX - 80.f, ScreenH * 0.5f + 120.f - Since * 20.f, nullptr, 1.1f);
		}
	}

	// ----- World event: objective panel (top-left) and a marker in the world -----
	const ANAGameState* Game = World ? World->GetGameState<ANAGameState>() : nullptr;
	if (Game && Game->IsEventActive())
	{
		const float Left = FMath::Max(0.f, Game->GetEventEndTime() - HudServerTime(World));
		DrawPanel(14.f, 40.f, 430.f, 58.f);
		DrawText(FString::Printf(TEXT("%s   %d:%02d"), *Game->GetEventTitle(), FMath::FloorToInt(Left / 60.f), FMath::FloorToInt(FMath::Fmod(Left, 60.f))),
			AshColor, 24.f, 44.f, nullptr, 1.f);
		DrawText(Game->GetEventDetail(), PowerText, 24.f, 66.f, nullptr, 0.8f);
		const FVector Where = Game->GetEventLocation();
		FVector2D Screen;
		if (!Where.IsNearlyZero() && ProjectToScreen(Where + FVector(0.f, 0.f, 200.f), Screen))
		{
			const float R = 10.f;
			DrawLine(Screen.X, Screen.Y - R, Screen.X + R, Screen.Y, AshColor, 2.f);
			DrawLine(Screen.X + R, Screen.Y, Screen.X, Screen.Y + R, AshColor, 2.f);
			DrawLine(Screen.X, Screen.Y + R, Screen.X - R, Screen.Y, AshColor, 2.f);
			DrawLine(Screen.X - R, Screen.Y, Screen.X, Screen.Y - R, AshColor, 2.f);
			DrawText(FString::Printf(TEXT("%.0f m"), FVector::Dist(Where, Player->GetActorLocation()) / 100.f), AshColor, Screen.X - 18.f, Screen.Y + 14.f, nullptr, 0.85f);
		}
	}

	if (Player->IsUsingBinoculars())
	{
		DrawBinocularScan(Player, ScreenW, ScreenH);
	}
}

void ANAHUD::DrawBinocularScan(ANAPlayerCharacter* Player, float ScreenW, float ScreenH)
{
	const float CenterX = ScreenW * 0.5f;
	const float CenterY = ScreenH * 0.5f;
	// Lens frame: dark bars top and bottom, a fine reticle.
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), 0.f, 0.f, ScreenW, ScreenH * 0.12f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), 0.f, ScreenH * 0.88f, ScreenW, ScreenH * 0.12f);
	DrawLine(CenterX - 40.f, CenterY, CenterX + 40.f, CenterY, FLinearColor(0.8f, 1.f, 0.8f, 0.6f), 1.f);
	DrawLine(CenterX, CenterY - 40.f, CenterX, CenterY + 40.f, FLinearColor(0.8f, 1.f, 0.8f, 0.6f), 1.f);

	const APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ViewDir = ViewRotation.Vector();

	// Pick the living thing closest to the centre of view (within ~3 degrees, 300 m, visible).
	AActor* Best = nullptr;
	float BestDot = FMath::Cos(FMath::DegreesToRadians(3.f));
	auto Consider = [&](AActor* Candidate)
	{
		const FVector To = Candidate->GetActorLocation() - ViewLocation;
		const float Distance = To.Size();
		if (Distance > 30000.f || Distance < 100.f)
		{
			return;
		}
		const float Dot = FVector::DotProduct(To / Distance, ViewDir);
		if (Dot <= BestDot)
		{
			return;
		}
		FHitResult Block;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(NABinoculars), false, Player);
		Params.AddIgnoredActor(Candidate);
		if (GetWorld()->LineTraceSingleByChannel(Block, ViewLocation, Candidate->GetActorLocation(), ECC_Visibility, Params))
		{
			return; // Hidden behind something.
		}
		Best = Candidate;
		BestDot = Dot;
	};
	for (TActorIterator<ANACreature> It(GetWorld()); It; ++It)       { if (!It->IsDead()) { Consider(*It); } }
	for (TActorIterator<ANABoss> It(GetWorld()); It; ++It)           { if (!It->IsDead()) { Consider(*It); } }
	for (TActorIterator<ANAHorse> It(GetWorld()); It; ++It)          { if (!It->IsDead()) { Consider(*It); } }
	for (TActorIterator<ANAEnemySoldier> It(GetWorld()); It; ++It)   { if (!It->IsDead()) { Consider(*It); } }

	TArray<FString> Lines;
	const UNAHealthComponent* TargetHealth = nullptr;
	if (const ANABoss* Boss = Cast<ANABoss>(Best))
	{
		Lines.Add(TEXT("ASHEN BEHEMOTH  (boss)"));
		Lines.Add(TEXT("Weak spots: glowing ember core on its back, head. Body armor halves damage."));
		Lines.Add(TEXT("Attacks: ground slam (red circle), charge, ember rain below 60%."));
		TargetHealth = Boss->GetHealthComponent();
	}
	else if (const ANACreature* Creature = Cast<ANACreature>(Best))
	{
		static const TCHAR* Moods[] = { TEXT("roaming"), TEXT("stalking"), TEXT("HUNTING"), TEXT("fleeing") };
		Lines.Add(FString::Printf(TEXT("%s  -  %s"), *Creature->GetSpeciesName().ToUpper(), Moods[FMath::Clamp(static_cast<int32>(Creature->GetMood()), 0, 3)]));
		Lines.Add(Creature->IsA<ANAWolf>() ? TEXT("Pack hunter: one wolf calls the others. Weak spot: head.")
			: TEXT("Ambush predator, bolder at night. Weak spot: the glowing eyes (head)."));
		Lines.Add(TEXT("Counter: bait distracts it, snare traps hold it, the bow hits it hard."));
		TargetHealth = Creature->GetHealthComponent();
	}
	else if (const ANAHorse* Mount = Cast<ANAHorse>(Best))
	{
		if (Mount->IsA<ANADeer>())
		{
			Lines.Add(TEXT("DEER  -  prey"));
			Lines.Add(TEXT("Very skittish: crouch to get close. Meat and hide."));
		}
		else
		{
			const bool bGlider = Mount->IsA<ANASkyMount>();
			Lines.Add(FString::Printf(TEXT("%s  -  %s"), bGlider ? TEXT("ASH GLIDER") : TEXT("HORSE"),
				Mount->IsTamed() ? *FString::Printf(TEXT("owned by %s"), *Mount->GetOwnerName()) : *FString::Printf(TEXT("wild, trust %.0f%%"), Mount->GetTrust() * 100.f)));
			Lines.Add(bGlider ? TEXT("Tame while perched with Raw/Cooked Meat (3x).") : TEXT("Approach slowly; tame with Berries or Herbs (3x)."));
		}
		TargetHealth = Mount->GetHealthComponent();
	}
	else if (const ANAEnemySoldier* Soldier = Cast<ANAEnemySoldier>(Best))
	{
		static const TCHAR* Alerts[] = { TEXT("unaware"), TEXT("suspicious"), TEXT("IN COMBAT") };
		Lines.Add(FString::Printf(TEXT("ENEMY SOLDIER  -  %s"), Alerts[FMath::Clamp(static_cast<int32>(Soldier->GetAlertState()), 0, 2)]));
		Lines.Add(TEXT("Assault rifle. Headshots kill fastest."));
		TargetHealth = Soldier->GetHealthComponent();
	}

	if (!Best)
	{
		DrawText(TEXT("SCANNING..."), FLinearColor(0.8f, 1.f, 0.8f, 0.7f), CenterX - 40.f, ScreenH * 0.12f + 10.f, nullptr, 0.9f);
		return;
	}
	const float Distance = FVector::Dist(Best->GetActorLocation(), Player->GetActorLocation()) / 100.f;
	float Y = ScreenH * 0.88f - 22.f * (Lines.Num() + 1) - 10.f;
	DrawPanel(CenterX - 300.f, Y - 6.f, 600.f, 22.f * (Lines.Num() + 1) + 12.f);
	for (const FString& Line : Lines)
	{
		DrawText(Line, FLinearColor(0.85f, 1.f, 0.85f), CenterX - 290.f, Y, nullptr, 0.9f);
		Y += 22.f;
	}
	const float HealthPercent = TargetHealth ? 100.f * TargetHealth->GetHealth() / FMath::Max(1.f, TargetHealth->GetMaxHealth()) : 0.f;
	DrawText(FString::Printf(TEXT("Distance %.0f m   Health %.0f%%"), Distance, HealthPercent), AshColor, CenterX - 290.f, Y, nullptr, 0.9f);
}

void ANAHUD::DrawPowers(ANAPlayerCharacter* Player, float ScreenW, float ScreenH)
{
	UWorld* World = GetWorld();
	const float CenterX = ScreenW * 0.5f;
	const float CenterY = ScreenH * 0.5f;
	const float ServerNow = HudServerTime(World);
	const APlayerController* PC = GetOwningPlayerController();
	const ANAPlayerState* State = PC ? PC->GetPlayerState<ANAPlayerState>() : nullptr;
	const UNAHealthComponent* Health = Player->GetHealthComponent();

	// ----- Downed: bleed-out and revive progress -----
	if (Player->IsDowned() && Health)
	{
		DrawRect(FLinearColor(0.25f, 0.f, 0.f, 0.4f), 0.f, 0.f, ScreenW, ScreenH);
		DrawText(TEXT("YOU ARE DOWN"), PowerAccent, CenterX - 95.f, CenterY - 120.f, nullptr, 2.f);
		const float Pool = FMath::Clamp(Health->GetHealth() / FMath::Max(Health->GetMaxHealth() * 0.5f, 1.f), 0.f, 1.f);
		DrawBar(CenterX - 150.f, CenterY - 80.f, 300.f, 14.f, Pool, FLinearColor(0.6f, 0.05f, 0.05f), TEXT("BLEEDING OUT"));
		if (Player->GetReviveProgress() > 0.f)
		{
			DrawBar(CenterX - 150.f, CenterY - 60.f, 300.f, 14.f, Player->GetReviveProgress(), FLinearColor(0.3f, 0.9f, 0.4f), TEXT("BEING REVIVED"));
		}
		DrawText(Player->IsBeingCarried() ? TEXT("A teammate is carrying you to safety") : TEXT("Crawl to cover. A teammate can revive or carry you."),
			PowerText, CenterX - 175.f, CenterY - 40.f, nullptr, 1.f);
	}

	// ----- Carrying / recovering / ash-touched -----
	float StatusY = CenterY + 140.f;
	auto Status = [this, CenterX, &StatusY](const FString& Text, const FLinearColor& Color)
	{
		DrawRect(PowerDim, CenterX - 180.f, StatusY, 360.f, 22.f);
		DrawText(Text, Color, CenterX - 170.f, StatusY + 3.f, nullptr, 0.95f);
		StatusY += 26.f;
	};
	if (Player->GetCarriedPlayer())
	{
		Status(TEXT("Carrying a teammate   [E] put down"), PowerText);
	}
	if (Player->IsRecovering())
	{
		Status(TEXT("Recovering from your wounds (slower)"), FLinearColor(0.8f, 0.8f, 0.5f));
	}
	if (State && State->GetAshTouchedUntil() > ServerNow)
	{
		Status(FString::Printf(TEXT("Ash-touched: slower for %d s"), FMath::CeilToInt(State->GetAshTouchedUntil() - ServerNow)), AshColor);
	}

	// ----- Lightning warnings: everyone sees where a strike is about to land -----
	for (TActorIterator<ANAPlayerCharacter> It(World); It; ++It)
	{
		const UNAAbilityComponent* Ability = It->GetAbilityComponent();
		if (!Ability)
		{
			continue;
		}
		FVector2D Screen;
		if (Ability->GetStrikeWarningEnd() > ServerNow && ProjectToScreen(Ability->GetStrikeLocation(), Screen))
		{
			const float R = 22.f;
			DrawLine(Screen.X - R, Screen.Y, Screen.X, Screen.Y - R, PowerAccent, 3.f);
			DrawLine(Screen.X, Screen.Y - R, Screen.X + R, Screen.Y, PowerAccent, 3.f);
			DrawLine(Screen.X + R, Screen.Y, Screen.X, Screen.Y + R, PowerAccent, 3.f);
			DrawLine(Screen.X, Screen.Y + R, Screen.X - R, Screen.Y, PowerAccent, 3.f);
			DrawText(TEXT("!! LIGHTNING"), PowerAccent, Screen.X - 45.f, Screen.Y + R + 4.f, nullptr, 1.f);
		}
		// Restoration Field: show the Guardian's field to everyone nearby.
		if (Ability->IsFieldActive() && ProjectToScreen(It->GetActorLocation() + FVector(0.f, 0.f, 120.f), Screen))
		{
			DrawText(TEXT("+ FIELD +"), FLinearColor(0.4f, 1.f, 0.5f), Screen.X - 30.f, Screen.Y, nullptr, 1.f);
		}
		if (*It != Player && Ability->IsFieldActive() && FVector::Dist(It->GetActorLocation(), Player->GetActorLocation()) <= 600.f)
		{
			Status(TEXT("Inside a Restoration Field: healing, faster revives"), FLinearColor(0.4f, 1.f, 0.5f));
		}
	}

	// ----- Boss: health bar at the top, and "get out" when you stand in a telegraph -----
	for (TActorIterator<ANABoss> It(World); It; ++It)
	{
		if (It->GetBossState() != ENABossState::Awake || FVector::Dist(It->GetActorLocation(), Player->GetActorLocation()) > 7000.f)
		{
			continue;
		}
		const UNAHealthComponent* BossHealth = It->GetHealthComponent();
		const float Fraction = BossHealth ? BossHealth->GetHealth() / FMath::Max(1.f, BossHealth->GetMaxHealth()) : 0.f;
		const float BarW = FMath::Min(640.f, ScreenW - 80.f);
		const float BarX = CenterX - BarW * 0.5f;
		DrawRect(PowerDim, BarX - 6.f, 162.f, BarW + 12.f, 44.f);
		const FString Title = FString::Printf(TEXT("ASHEN BEHEMOTH%s"), It->GetPhase() >= 3 ? TEXT("  -  ENRAGED") : It->GetPhase() == 2 ? TEXT("  -  EMBER RAIN") : TEXT(""));
		DrawText(Title, AshColor, BarX, 164.f, nullptr, 1.05f);
		DrawRect(FLinearColor(0.1f, 0.02f, 0.02f, 0.9f), BarX, 184.f, BarW, 14.f);
		DrawRect(It->GetPhase() >= 3 ? FLinearColor(1.f, 0.15f, 0.05f) : FLinearColor(0.85f, 0.3f, 0.05f), BarX, 184.f, BarW * FMath::Clamp(Fraction, 0.f, 1.f), 14.f);
		DrawText(TEXT("Weak points: the glowing core on its back, and the head"), PowerText, BarX, 200.f, nullptr, 0.8f);
		for (int32 Index = 0; Index < It->NumWarnings(); ++Index)
		{
			if (It->GetWarningEnd(Index) > ServerNow &&
				FVector::Dist2D(It->GetWarningLocation(Index), Player->GetActorLocation()) <= It->GetWarningRadius(Index))
			{
				DrawText(TEXT("!! GET OUT OF THE RED CIRCLE"), PowerAccent, CenterX - 140.f, CenterY - 160.f, nullptr, 1.5f);
				break;
			}
		}
	}

	// ----- Downed teammates: show where they are -----
	for (TActorIterator<ANAPlayerCharacter> It(World); It; ++It)
	{
		FVector2D Screen;
		if (*It != Player && It->IsDowned() && !It->IsDead() && ProjectToScreen(It->GetActorLocation() + FVector(0.f, 0.f, 60.f), Screen))
		{
			DrawText(FString::Printf(TEXT("+ DOWN %.0fm"), FVector::Dist(It->GetActorLocation(), Player->GetActorLocation()) / 100.f),
				FLinearColor(1.f, 0.85f, 0.2f), Screen.X - 35.f, Screen.Y, nullptr, 1.f);
		}
	}

	UNAAbilityComponent* MyAbility = Player->GetAbilityComponent();
	if (!MyAbility)
	{
		return;
	}

	// ----- Hunter tracking: enemies and creatures through walls -----
	if (MyAbility->IsTrackingActive())
	{
		auto Mark = [this, Player](const AActor* Target, const FLinearColor& Color, const TCHAR* Label)
		{
			const float TrackRange = 8000.f; // 80 m
			FVector2D Screen;
			if (FVector::Dist(Target->GetActorLocation(), Player->GetActorLocation()) <= TrackRange && ProjectToScreen(Target->GetActorLocation(), Screen))
			{
				DrawRect(Color, Screen.X - 5.f, Screen.Y - 5.f, 10.f, 10.f);
				DrawText(Label, Color, Screen.X + 8.f, Screen.Y - 8.f, nullptr, 0.85f);
			}
		};
		for (TActorIterator<ANAEnemySoldier> It(World); It; ++It)
		{
			if (!It->IsDead()) { Mark(*It, PowerAccent, TEXT("ENEMY")); }
		}
		for (TActorIterator<ANACreature> It(World); It; ++It)
		{
			if (!It->IsDead()) { Mark(*It, AshColor, TEXT("CREATURE")); }
		}
		for (TActorIterator<ANABoss> It(World); It; ++It)
		{
			if (!It->IsDead()) { Mark(*It, PowerAccent, TEXT("BOSS")); }
		}
		// Fresh tracks within 30 m light up, so the Hunter can follow the trail.
		for (TActorIterator<ANACreatureTrack> It(World); It; ++It)
		{
			FVector2D Screen;
			if (FVector::Dist(It->GetActorLocation(), Player->GetActorLocation()) <= 3000.f && ProjectToScreen(It->GetActorLocation(), Screen))
			{
				const float Fresh = 1.f - FMath::Clamp(It->GetAgeSeconds() / ANACreatureTrack::LifeSeconds, 0.f, 1.f);
				DrawRect(FLinearColor(1.f, 0.75f, 0.3f, 0.3f + 0.6f * Fresh), Screen.X - 3.f, Screen.Y - 3.f, 6.f, 6.f);
			}
		}
		for (TActorIterator<ANAPlayerCharacter> It(World); It; ++It)
		{
			if (*It != Player && !It->IsDead() && !It->IsCloaked()) { Mark(*It, FLinearColor(0.4f, 0.8f, 1.f), TEXT("PLAYER")); }
		}
	}

	// ----- Class power and Ash charges (bottom centre) -----
	const ENAArchetype Archetype = MyAbility->GetArchetype();
	const float PanelX = CenterX - 150.f;
	const float PanelY = ScreenH - 150.f;
	DrawRect(PowerDim, PanelX - 8.f, PanelY - 6.f, 316.f, 54.f);
	const float Cooldown = MyAbility->GetCooldownRemaining();
	const float Active = MyAbility->GetActiveRemaining();
	FString PowerLine = FString::Printf(TEXT("[Q] %s"), *UNAAbilityComponent::AbilityName(Archetype));
	if (Active > 0.f && Archetype != ENAArchetype::Storm)
	{
		PowerLine += FString::Printf(TEXT("   ACTIVE %.0fs"), FMath::CeilToFloat(Active));
	}
	else if (Cooldown > 0.f)
	{
		PowerLine += FString::Printf(TEXT("   %.0fs"), FMath::CeilToFloat(Cooldown));
	}
	else
	{
		PowerLine += TEXT("   READY");
	}
	DrawText(PowerLine, Cooldown > 0.f ? PowerText : FLinearColor(0.5f, 1.f, 0.6f), PanelX, PanelY, nullptr, 1.f);
	DrawBar(PanelX, PanelY + 18.f, 300.f, 8.f, MyAbility->GetEnergyFraction(), FLinearColor(0.55f, 0.35f, 0.95f), FString());
	if (State)
	{
		const FString Shrine = State->GetAttunedShrineName().IsEmpty() ? TEXT("not attuned") : State->GetAttunedShrineName();
		DrawText(FString::Printf(TEXT("ASH %d/%d  -  %s"), State->GetAshCharges(), ANAPlayerState::MaxAshCharges, *Shrine),
			AshColor, PanelX, PanelY + 30.f, nullptr, 0.85f);
	}
	if (MyAbility->IsCloaked())
	{
		DrawRect(FLinearColor(0.3f, 0.35f, 0.5f, 0.18f), 0.f, 0.f, ScreenW, ScreenH);
		DrawText(TEXT("CLOAKED - shooting or taking damage reveals you"), FLinearColor(0.7f, 0.8f, 1.f), CenterX - 170.f, 140.f, nullptr, 1.f);
	}
}
