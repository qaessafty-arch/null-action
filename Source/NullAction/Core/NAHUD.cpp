#include "Core/NAHUD.h"

#include "AI/NAEnemySoldier.h"
#include "Character/NAPlayerCharacter.h"
#include "Creatures/NACreature.h"
#include "EngineUtils.h"
#include "Combat/NAHealthComponent.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/NAInteractable.h"
#include "Survival/NASurvivalComponent.h"
#include "Weapons/NAWeapon.h"

namespace
{
	// NULL-ACTION palette.
	const FLinearColor Accent(0.95f, 0.22f, 0.16f);
	const FLinearColor TextColor(0.92f, 0.92f, 0.92f);
	const FLinearColor Dim(0.f, 0.f, 0.f, 0.55f);

	const TCHAR* GaitName(ENAGait Gait)
	{
		switch (Gait)
		{
		case ENAGait::Walk:   return TEXT("Walk");
		case ENAGait::Run:    return TEXT("Run");
		case ENAGait::Sprint: return TEXT("Sprint");
		}
		return TEXT("?");
	}

	const TCHAR* NetModeName(ENetMode Mode)
	{
		switch (Mode)
		{
		case NM_Standalone:      return TEXT("Standalone");
		case NM_DedicatedServer: return TEXT("Dedicated Server");
		case NM_ListenServer:    return TEXT("Listen Server");
		case NM_Client:          return TEXT("Client");
		default:                 return TEXT("?");
		}
	}
}

void ANAHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color, const FString& Label)
{
	DrawRect(Dim, X, Y, Width, Height);
	DrawRect(Color, X + 2.f, Y + 2.f, (Width - 4.f) * FMath::Clamp(Fraction, 0.f, 1.f), Height - 4.f);
	DrawText(Label, TextColor, X + Width + 8.f, Y - 1.f, nullptr, 0.9f);
}

void ANAHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float ScreenW = Canvas->ClipX;
	const float ScreenH = Canvas->ClipY;
	const float CenterX = ScreenW * 0.5f;
	const float CenterY = ScreenH * 0.5f;

	APlayerController* PC = GetOwningPlayerController();
	ANAPlayerCharacter* Player = PC ? Cast<ANAPlayerCharacter>(PC->GetPawn()) : nullptr;

	// ----- Dead: show the respawn message over your body -----
	if (!Player)
	{
		const ANACharacterBase* Watched = PC ? Cast<ANACharacterBase>(PC->GetViewTarget()) : nullptr;
		if (Watched && Watched->IsDead())
		{
			DrawRect(FLinearColor(0.15f, 0.f, 0.f, 0.35f), 0.f, 0.f, ScreenW, ScreenH);
			DrawText(TEXT("YOU WERE KILLED"), Accent, CenterX - 110.f, CenterY - 30.f, nullptr, 2.f);
			DrawText(TEXT("Respawning..."), TextColor, CenterX - 50.f, CenterY + 15.f, nullptr, 1.1f);
		}
		return;
	}

	const UNAHealthComponent* Health = Player->GetHealthComponent();
	const UNASurvivalComponent* Survival = Player->GetSurvivalComponent();
	const ANAWeapon* Weapon = Player->GetCurrentWeapon();

	// ----- Damage flash -----
	if (Health)
	{
		const float SinceHurt = Now - Health->GetLastDamageTime();
		if (SinceHurt < 0.6f)
		{
			DrawRect(FLinearColor(0.6f, 0.f, 0.f, 0.35f * (1.f - SinceHurt / 0.6f)), 0.f, 0.f, ScreenW, ScreenH);
		}
	}

	// ----- Help text (top-left) -----
	if (bShowDebugOverlay)
	{
		float Y = 24.f;
		auto Line = [this, &Y](const FString& Text, const FLinearColor& Color)
		{
			DrawText(Text, Color, 24.f, Y, nullptr, 0.9f);
			Y += 16.f;
		};
		Line(TEXT("NULL-ACTION  //  dev build 0.4"), Accent);
		Line(TEXT("WASD move  Mouse look  Shift sprint  Alt walk  C crouch  Space jump"), TextColor);
		Line(TEXT("LMB fire  RMB aim  R reload  E interact  1-4 / wheel switch weapon"), TextColor);
		Line(FString::Printf(TEXT("Gait: %s%s   Speed: %.0f cm/s   %s"),
			GaitName(Player->GetGait()), Player->bIsCrouched ? TEXT(" (crouched)") : TEXT(""),
			Player->GetVelocity().Size2D(), NetModeName(GetNetMode())), TextColor);
	}

	// ----- Crosshair: gap grows with the weapon's current spread -----
	const float Spread = Weapon ? Weapon->GetCurrentSpreadDegrees() : 0.f;
	const float Gap = 5.f + Spread * 5.f;
	const float Length = 9.f;
	const FLinearColor CrossColor = Player->GetFocusedInteractable() ? Accent : TextColor;
	if (Weapon)
	{
		DrawLine(CenterX - Gap - Length, CenterY, CenterX - Gap, CenterY, CrossColor, 2.f);
		DrawLine(CenterX + Gap, CenterY, CenterX + Gap + Length, CenterY, CrossColor, 2.f);
		DrawLine(CenterX, CenterY - Gap - Length, CenterX, CenterY - Gap, CrossColor, 2.f);
		DrawLine(CenterX, CenterY + Gap, CenterX, CenterY + Gap + Length, CrossColor, 2.f);
	}
	else
	{
		DrawRect(CrossColor, CenterX - 2.f, CenterY - 2.f, 4.f, 4.f);
	}

	// ----- Hit marker and damage number -----
	const float SinceHit = Now - Player->GetLastHitConfirmTime();
	if (SinceHit < 0.35f)
	{
		const FLinearColor HitColor = Player->WasLastHitHeadshot() ? Accent : TextColor;
		const float A = 6.f, B = 14.f;
		DrawLine(CenterX - B, CenterY - B, CenterX - A, CenterY - A, HitColor, 2.f);
		DrawLine(CenterX + A, CenterY - A, CenterX + B, CenterY - B, HitColor, 2.f);
		DrawLine(CenterX - B, CenterY + B, CenterX - A, CenterY + A, HitColor, 2.f);
		DrawLine(CenterX + A, CenterY + A, CenterX + B, CenterY + B, HitColor, 2.f);

		FString HitText = FString::Printf(TEXT("-%.0f"), Player->GetLastHitDamage());
		if (Player->WasLastHitHeadshot()) { HitText += TEXT("  HEADSHOT"); }
		if (Player->WasLastHitKill())     { HitText += TEXT("  KILL"); }
		DrawText(HitText, HitColor, CenterX + 22.f, CenterY - 30.f - SinceHit * 40.f, nullptr, 1.1f);
	}

	// ----- Enemy warnings (alert state is replicated from the server) -----
	int32 Engaging = 0;
	int32 Suspicious = 0;
	for (TActorIterator<ANAEnemySoldier> It(GetWorld()); It; ++It)
	{
		if (It->IsDead() || It->GetAlertTarget() != Player)
		{
			continue;
		}
		if (It->GetAlertState() == ENAAlertState::Combat)
		{
			++Engaging;
		}
		else if (It->GetAlertState() == ENAAlertState::Suspicious)
		{
			++Suspicious;
		}
	}
	bool bHunted = false;
	bool bStalked = false;
	for (TActorIterator<ANACreature> It(GetWorld()); It; ++It)
	{
		if (It->IsDead() || It->GetPrey() != Player)
		{
			continue;
		}
		bHunted |= It->GetMood() == ENACreatureMood::Hunting;
		bStalked |= It->GetMood() == ENACreatureMood::Stalking;
	}

	if (bHunted)
	{
		DrawRect(FLinearColor(0.55f, 0.15f, 0.f, 0.7f), CenterX - 150.f, 108.f, 300.f, 30.f);
		DrawText(TEXT("!!  SOMETHING IS HUNTING YOU"), TextColor, CenterX - 135.f, 113.f, nullptr, 1.2f);
	}
	else if (bStalked)
	{
		// Deliberately subtle: a stalker should be unsettling, not announced.
		DrawText(TEXT("...you feel watched"), FLinearColor(0.8f, 0.6f, 0.4f, 0.6f), CenterX - 70.f, 113.f, nullptr, 1.f);
	}

	if (Engaging > 0)
	{
		const FString Warning = Engaging == 1 ? TEXT("!  ENEMY ENGAGING") : FString::Printf(TEXT("!  %d ENEMIES ENGAGING"), Engaging);
		DrawRect(FLinearColor(0.5f, 0.02f, 0.02f, 0.65f), CenterX - 130.f, 70.f, 260.f, 30.f);
		DrawText(Warning, TextColor, CenterX - 110.f, 75.f, nullptr, 1.2f);
	}
	else if (Suspicious > 0)
	{
		DrawRect(FLinearColor(0.45f, 0.35f, 0.02f, 0.6f), CenterX - 130.f, 70.f, 260.f, 30.f);
		DrawText(TEXT("?  YOU ARE BEING NOTICED"), TextColor, CenterX - 115.f, 75.f, nullptr, 1.1f);
	}

	// ----- Interaction prompt -----
	if (const INAInteractable* Interactable = Cast<INAInteractable>(Player->GetFocusedInteractable()))
	{
		const FString Prompt = FString::Printf(TEXT("[E]  %s"), *Interactable->GetInteractionPrompt(Player).ToString());
		DrawRect(Dim, CenterX - 120.f, CenterY + 60.f, 240.f, 26.f);
		DrawText(Prompt, TextColor, CenterX - 110.f, CenterY + 64.f, nullptr, 1.f);
	}

	// ----- Vitals (bottom-left) -----
	const float BarX = 28.f;
	const float BarW = 220.f;
	const float BarH = 14.f;
	float BarY = ScreenH - 130.f;
	if (Health)
	{
		DrawBar(BarX, BarY, BarW, BarH, Health->GetHealth() / FMath::Max(Health->GetMaxHealth(), 1.f),
			FLinearColor(0.8f, 0.12f, 0.1f), FString::Printf(TEXT("HEALTH %.0f"), Health->GetHealth()));
		BarY += 20.f;
		DrawBar(BarX, BarY, BarW, BarH, Health->GetArmor() / FMath::Max(Health->GetMaxArmor(), 1.f),
			FLinearColor(0.2f, 0.4f, 0.85f), FString::Printf(TEXT("ARMOR %.0f"), Health->GetArmor()));
		BarY += 20.f;
	}
	if (Survival)
	{
		DrawBar(BarX, BarY, BarW, BarH, Survival->GetStaminaFraction(),
			Survival->IsExhausted() ? FLinearColor(0.5f, 0.4f, 0.1f) : FLinearColor(0.95f, 0.8f, 0.2f),
			Survival->IsExhausted() ? TEXT("STAMINA (exhausted)") : TEXT("STAMINA"));
		BarY += 20.f;
		DrawBar(BarX, BarY, BarW * 0.5f, BarH - 4.f, Survival->GetHungerFraction(), FLinearColor(0.85f, 0.45f, 0.1f),
			FString::Printf(TEXT("FOOD %.0f%%"), Survival->GetHungerFraction() * 100.f));
		BarY += 16.f;
		DrawBar(BarX, BarY, BarW * 0.5f, BarH - 4.f, Survival->GetThirstFraction(), FLinearColor(0.15f, 0.65f, 0.85f),
			FString::Printf(TEXT("WATER %.0f%%"), Survival->GetThirstFraction() * 100.f));
	}

	// ----- Weapon and ammo (bottom-right) -----
	if (Weapon)
	{
		const float RightX = ScreenW - 260.f;
		DrawRect(Dim, RightX - 12.f, ScreenH - 108.f, 250.f, 78.f);
		DrawText(Weapon->GetStats().DisplayName.ToString().ToUpper(), Accent, RightX, ScreenH - 100.f, nullptr, 1.f);
		DrawText(FString::Printf(TEXT("%d / %d"), Weapon->GetAmmoInMagazine(), Weapon->GetReserveAmmo()),
			TextColor, RightX, ScreenH - 80.f, nullptr, 1.8f);
		if (Weapon->IsReloading())
		{
			DrawText(TEXT("RELOADING"), Accent, RightX + 130.f, ScreenH - 74.f, nullptr, 1.f);
		}
		else if (Weapon->GetAmmoInMagazine() == 0)
		{
			DrawText(Weapon->GetReserveAmmo() > 0 ? TEXT("PRESS R") : TEXT("NO AMMO"), Accent, RightX + 130.f, ScreenH - 74.f, nullptr, 1.f);
		}
	}

	// ----- Weapon slots -----
	float SlotX = ScreenW - 260.f;
	for (const ANAWeapon* Carried : Player->GetWeapons())
	{
		if (!Carried)
		{
			continue;
		}
		const bool bCurrent = Carried == Weapon;
		DrawText(FString::Printf(TEXT("%d %s"), Carried->GetStats().Slot, *Carried->GetStats().DisplayName.ToString()),
			bCurrent ? Accent : FLinearColor(0.6f, 0.6f, 0.6f), SlotX, ScreenH - 130.f, nullptr, 0.8f);
		SlotX += 62.f;
	}
}
