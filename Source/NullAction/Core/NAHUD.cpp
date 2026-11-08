#include "Core/NAHUD.h"

#include "AI/NAEnemySoldier.h"
#include "Character/NAPlayerCharacter.h"
#include "Core/NAGameState.h"
#include "Core/NAPlayerController.h"
#include "Core/NAPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Creatures/NACreature.h"
#include "EngineUtils.h"
#include "Combat/NAHealthComponent.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/NAInteractable.h"
#include "Survival/NASurvivalComponent.h"
#include "Weapons/NAWeapon.h"
#include "World/NAWorldEnvironment.h"
#include "World/NATerrainManager.h"
#include "Inventory/NAInventoryComponent.h"
#include "Inventory/NAItems.h"

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

void ANAHUD::DrawChat(float ScreenH)
{
	const ANAPlayerController* PC = Cast<ANAPlayerController>(GetOwningPlayerController());
	if (!PC)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const TArray<FNAChatLine>& ChatLines = PC->GetChatLog();
	float Y = ScreenH - 170.f;
	int32 Shown = 0;
	for (int32 Index = ChatLines.Num() - 1; Index >= 0 && Shown < 7; --Index)
	{
		const FNAChatLine& Line = ChatLines[Index];
		const float Age = Now - Line.LocalTime;
		if (Age > 25.f && !bShowScoreboard)
		{
			break; // Old lines fade out (hold Tab to see them again).
		}
		FString Prefix;
		FLinearColor Color = TextColor;
		switch (Line.Channel)
		{
		case ENAChatChannel::Team:      Prefix = TEXT("[TEAM] ");  Color = FLinearColor(0.5f, 0.8f, 1.f); break;
		case ENAChatChannel::Proximity: Prefix = TEXT("[NEAR] ");  Color = FLinearColor(0.8f, 1.f, 0.7f); break;
		case ENAChatChannel::Private:   Prefix = TEXT("[PM] ");    Color = FLinearColor(1.f, 0.6f, 1.f);  break;
		case ENAChatChannel::System:    Prefix = TEXT("* ");       Color = FLinearColor(1.f, 0.85f, 0.4f); break;
		default: break;
		}
		const FString Text = Line.Sender.IsEmpty() ? Prefix + Line.Text : FString::Printf(TEXT("%s%s: %s"), *Prefix, *Line.Sender, *Line.Text);
		DrawText(Text, Color, 28.f, Y, nullptr, 0.9f);
		Y -= 16.f;
		++Shown;
	}
}

void ANAHUD::DrawKillFeed(float ScreenW)
{
	const ANAGameState* State = GetWorld()->GetGameState<ANAGameState>();
	if (!State)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	float Y = 24.f;
	for (const FNAKillFeedEntry& Entry : State->GetKillFeed())
	{
		if (Now - Entry.LocalTime > 8.f)
		{
			continue;
		}
		const FString Text = FString::Printf(TEXT("%s  >  %s"), *Entry.Killer, *Entry.Victim);
		DrawRect(Dim, ScreenW - 330.f, Y - 2.f, 310.f, 20.f);
		DrawText(Text, TextColor, ScreenW - 320.f, Y, nullptr, 0.9f);
		Y += 22.f;
	}
}

void ANAHUD::DrawScoreboard(float ScreenW, float ScreenH)
{
	const AGameStateBase* State = GetWorld()->GetGameState();
	if (!State)
	{
		return;
	}
	const float Width = 560.f;
	const float X = (ScreenW - Width) * 0.5f;
	float Y = ScreenH * 0.22f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.75f), X, Y, Width, 40.f + 24.f * State->PlayerArray.Num());
	DrawText(TEXT("NULL-ACTION  //  PLAYERS"), Accent, X + 16.f, Y + 10.f, nullptr, 1.1f);
	DrawText(TEXT("K     D     PING"), TextColor, X + Width - 170.f, Y + 12.f, nullptr, 0.9f);
	Y += 40.f;
	for (const APlayerState* Listed : State->PlayerArray)
	{
		const ANAPlayerState* NAState = Cast<ANAPlayerState>(Listed);
		if (!NAState)
		{
			continue;
		}
		const TCHAR* Tag = NAState->GetRole() == ENAPlayerRole::Owner ? TEXT("  [OWNER]")
			: NAState->GetRole() == ENAPlayerRole::Admin ? TEXT("  [ADMIN]")
			: NAState->GetRole() == ENAPlayerRole::Moderator ? TEXT("  [MOD]") : TEXT("");
		const bool bMe = GetOwningPlayerController() && GetOwningPlayerController()->PlayerState == Listed;
		DrawText(NAState->GetPlayerName() + Tag, bMe ? Accent : TextColor, X + 16.f, Y, nullptr, 1.f);
		DrawText(FString::Printf(TEXT("%-5d %-5d %.0f"), NAState->GetKills(), NAState->GetDeaths(), NAState->GetPingInMilliseconds()),
			TextColor, X + Width - 170.f, Y, nullptr, 1.f);
		Y += 24.f;
	}
}

void ANAHUD::DrawInventory(ANAPlayerCharacter* Player, float ScreenW, float ScreenH)
{
	const UNAInventoryComponent* Inventory = Player ? Player->GetInventoryComponent() : nullptr;
	if (!Inventory)
	{
		return;
	}

	const float Width = 620.f;
	const float Height = 430.f;
	const float X = (ScreenW - Width) * 0.5f;
	const float Y = (ScreenH - Height) * 0.5f;
	DrawRect(FLinearColor(0.02f, 0.02f, 0.03f, 0.88f), X, Y, Width, Height);
	DrawRect(Accent, X, Y, Width, 3.f);

	const bool bItemsTab = Player->GetInventoryTab() == 0;
	DrawText(TEXT("INVENTORY"), bItemsTab ? Accent : FLinearColor(0.5f, 0.5f, 0.5f), X + 20.f, Y + 14.f, nullptr, 1.2f);
	DrawText(TEXT("CRAFTING"), !bItemsTab ? Accent : FLinearColor(0.5f, 0.5f, 0.5f), X + 180.f, Y + 14.f, nullptr, 1.2f);
	DrawText(TEXT("Left/Right: tab   Up/Down: select   Enter: use / craft / build   I: close"), FLinearColor(0.6f, 0.6f, 0.6f),
		X + 20.f, Y + Height - 26.f, nullptr, 0.85f);

	float RowY = Y + 52.f;
	const int32 Selected = Player->GetInventoryIndex();

	if (bItemsTab)
	{
		const TArray<FNAItemStack>& Stacks = Inventory->GetItems();
		if (Stacks.Num() == 0)
		{
			DrawText(TEXT("Empty. Chop trees, mine rocks, pick herbs and berries, salvage scrap, hunt."), TextColor, X + 20.f, RowY, nullptr, 0.95f);
		}
		for (int32 Index = 0; Index < Stacks.Num() && Index < 13; ++Index)
		{
			const FNAItemDef* Def = NAItems::Find(Stacks[Index].ItemId);
			const bool bSelected = Index == Selected;
			if (bSelected)
			{
				DrawRect(FLinearColor(0.3f, 0.06f, 0.04f, 0.8f), X + 12.f, RowY - 2.f, Width - 24.f, 24.f);
			}
			DrawRect(Def ? Def->Color : FLinearColor::White, X + 20.f, RowY + 3.f, 14.f, 14.f);
			DrawText(FString::Printf(TEXT("%s  x%d"), Def ? *Def->DisplayName : *Stacks[Index].ItemId.ToString(), Stacks[Index].Count),
				bSelected ? Accent : TextColor, X + 44.f, RowY, nullptr, 1.f);
			if (bSelected && Def)
			{
				DrawText(Def->Description, FLinearColor(0.75f, 0.75f, 0.75f), X + 280.f, RowY + 2.f, nullptr, 0.85f);
			}
			RowY += 26.f;
		}
		return;
	}

	const TArray<FNARecipe>& Recipes = NAItems::Recipes();
	const bool bNearFire = Inventory->IsNearCampfire();
	for (int32 Index = 0; Index < Recipes.Num(); ++Index)
	{
		const FNARecipe& Recipe = Recipes[Index];
		const bool bSelected = Index == Selected;
		const bool bCanCraft = Inventory->CanCraft(Recipe);
		if (bSelected)
		{
			DrawRect(FLinearColor(0.3f, 0.06f, 0.04f, 0.8f), X + 12.f, RowY - 2.f, Width - 24.f, 24.f);
		}
		FString Needs;
		for (const FNAIngredient& Input : Recipe.Inputs)
		{
			Needs += FString::Printf(TEXT("%s%d %s (%d)"), Needs.IsEmpty() ? TEXT("") : TEXT(", "), Input.Count,
				*NAItems::NameOf(Input.Item), Inventory->CountOf(Input.Item));
		}
		if (Recipe.bNeedsCampfire)
		{
			Needs += bNearFire ? TEXT("  [campfire OK]") : TEXT("  [needs campfire]");
		}
		DrawText(NAItems::NameOf(Recipe.Output), bCanCraft ? FLinearColor(0.5f, 1.f, 0.5f) : FLinearColor(0.6f, 0.6f, 0.6f), X + 20.f, RowY, nullptr, 1.f);
		DrawText(Needs, bSelected ? TextColor : FLinearColor(0.6f, 0.6f, 0.6f), X + 200.f, RowY + 2.f, nullptr, 0.85f);
		RowY += 26.f;
	}
}

void ANAHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	DrawChat(Canvas->ClipY);
	DrawKillFeed(Canvas->ClipX);
	if (bShowScoreboard)
	{
		DrawScoreboard(Canvas->ClipX, Canvas->ClipY);
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
			DrawDeathScreen(ScreenW, ScreenH);
		}
		return;
	}

	const UNAHealthComponent* Health = Player->GetHealthComponent();
	const UNASurvivalComponent* Survival = Player->GetSurvivalComponent();
	const ANAWeapon* Weapon = Player->GetCurrentWeapon();

	// ----- Time and weather (top centre) and weather tint -----
	if (const ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this))
	{
		const FNAWeatherProfile Weather = Environment->GetBlendedProfile();
		if (Weather.Tint.A > 0.01f)
		{
			DrawRect(Weather.Tint, 0.f, 0.f, ScreenW, ScreenH);
		}
		const float Hours = Environment->GetTimeOfDay();
		const int32 Hour = FMath::FloorToInt(Hours);
		const int32 Minute = FMath::FloorToInt((Hours - Hour) * 60.f);
		const FString Clock = FString::Printf(TEXT("%02d:%02d   %s   %.0f C%s"), Hour, Minute,
			ANAWorldEnvironment::WeatherName(Environment->GetWeather()), Environment->GetAmbientTemperature(),
			Environment->IsNight() ? TEXT("   NIGHT") : TEXT(""));
		DrawRect(Dim, CenterX - 120.f, 18.f, 240.f, 24.f);
		DrawText(Clock, TextColor, CenterX - 105.f, 21.f, nullptr, 1.f);
		// Region name under the clock.
		const FString Region = NATerrain::RegionAt(Player->GetActorLocation());
		DrawText(Region.ToUpper(), Accent, CenterX - Region.Len() * 4.5f, 46.f, nullptr, 0.95f);
	}

	// ----- Underwater tint -----
	if (Player->IsCameraUnderwater())
	{
		DrawRect(FLinearColor(0.f, 0.12f, 0.22f, 0.45f), 0.f, 0.f, ScreenW, ScreenH);
	}

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
		Line(TEXT("NULL-ACTION  //  dev build 1.0"), Accent);
		Line(TEXT("WASD move  Mouse look  Shift sprint  Alt walk  C crouch  Space jump"), TextColor);
		Line(TEXT("LMB fire  RMB aim  R reload  E interact  1-4 / wheel switch weapon"), TextColor);
		Line(TEXT("Z prone  V dodge  Shift+C slide  Space at a wall: vault/climb  In water: Space up, C down"), TextColor);
		Line(TEXT("I inventory & crafting   E gather (trees, rocks, herbs, berries, scrap)"), TextColor);
		Line(TEXT("Q class power   Console: NAClass Phantom|Hunter|Storm|Guardian   Downed teammate: tap E to revive, Crouch+E to carry"), TextColor);
		Line(TEXT("Tab scoreboard   ~ console: NAChat \"hi\"  NAPlayers  (owner: NAGod NAFly NASpawn NAGive NAWeather NATime ...)"), TextColor);
		if (const ANAPlayerState* MyState = PC ? PC->GetPlayerState<ANAPlayerState>() : nullptr)
		{
			if (MyState->GetRole() != ENAPlayerRole::Player)
			{
				FString Powers = MyState->GetRole() == ENAPlayerRole::Owner ? TEXT("ROLE: OWNER") : MyState->GetRole() == ENAPlayerRole::Admin ? TEXT("ROLE: ADMIN") : TEXT("ROLE: MODERATOR");
				if (Player->IsGodMode())     { Powers += TEXT("   GOD"); }
				if (Player->IsAdminFlying()) { Powers += TEXT("   FLY"); }
				Line(Powers, Accent);
			}
		}
		const TCHAR* Stance = Player->IsSwimming() ? TEXT(" (swimming)") : Player->IsSliding() ? TEXT(" (sliding)")
			: Player->IsProne() ? TEXT(" (prone)") : Player->bIsCrouched ? TEXT(" (crouched)") : TEXT("");
		Line(FString::Printf(TEXT("Gait: %s%s   Speed: %.0f cm/s   %s"),
			GaitName(Player->GetGait()), Stance,
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

	// ----- Interaction / gathering prompt -----
	FString Prompt;
	if (const INAInteractable* Interactable = Cast<INAInteractable>(Player->GetFocusedInteractable()))
	{
		Prompt = FString::Printf(TEXT("[E]  %s"), *Interactable->GetInteractionPrompt(Player).ToString());
	}
	else if (!Player->GetGatherPrompt().IsEmpty())
	{
		Prompt = FString::Printf(TEXT("[E]  %s"), *Player->GetGatherPrompt());
	}
	if (!Prompt.IsEmpty())
	{
		DrawRect(Dim, CenterX - 140.f, CenterY + 60.f, 280.f, 26.f);
		DrawText(Prompt, TextColor, CenterX - 130.f, CenterY + 64.f, nullptr, 1.f);
	}

	// ----- Build mode -----
	if (Player->IsBuilding())
	{
		const FString BuildText = FString::Printf(TEXT("BUILDING: %s   LMB place   R rotate   RMB cancel%s"),
			*NAItems::NameOf(Player->GetBuildItem()), Player->IsPlacementValid() ? TEXT("") : TEXT("   (can't place here)"));
		DrawRect(Dim, CenterX - 260.f, CenterY + 100.f, 520.f, 26.f);
		DrawText(BuildText, Player->IsPlacementValid() ? FLinearColor(0.5f, 1.f, 0.5f) : Accent, CenterX - 250.f, CenterY + 104.f, nullptr, 0.95f);
	}

	// ----- Downed, revive, carry, class power, Ash charges -----
	DrawPowers(Player, ScreenW, ScreenH);

	// ----- Inventory / crafting panel -----
	if (Player->IsInventoryOpen())
	{
		DrawInventory(Player, ScreenW, ScreenH);
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
		BarY += 16.f;

		const float Temperature = Survival->GetBodyTemperature();
		const FLinearColor TempColor = Survival->IsHypothermic() ? FLinearColor(0.4f, 0.7f, 1.f) : TextColor;
		DrawText(FString::Printf(TEXT("BODY %.1f C%s"), Temperature, Survival->IsHypothermic() ? TEXT("  (cold!)") : TEXT("")),
			TempColor, BarX, BarY - 2.f, nullptr, 0.85f);

		// Oxygen only appears when it matters.
		if (Survival->GetOxygenFraction() < 0.999f || Player->IsHeadUnderwater())
		{
			const float Oxygen = Survival->GetOxygenFraction();
			const FLinearColor OxygenColor = Oxygen < 0.25f ? Accent : FLinearColor(0.75f, 0.9f, 1.f);
			DrawBar(CenterX - 110.f, ScreenH - 70.f, 220.f, 12.f, Oxygen, OxygenColor,
				Oxygen <= 0.f ? TEXT("DROWNING!") : TEXT("OXYGEN"));
		}
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
