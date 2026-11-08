// HUD pieces for Milestone 5B: downed/revive/carry, Ash Rebirth info and class powers.
#include "Core/NAHUD.h"

#include "Abilities/NAAbilityComponent.h"
#include "Character/NAPlayerCharacter.h"
#include "Combat/NAHealthComponent.h"
#include "Core/NAPlayerState.h"
#include "Creatures/NACreature.h"
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
