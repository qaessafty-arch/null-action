// Menus drawn with Canvas: intro captions, main menu, character studio, settings, info screens, pause.
#include "Core/NAHUD.h"

#include "Character/NAPlayerCharacter.h"
#include "Core/NAGameState.h"
#include "Core/NAPlayerController.h"
#include "Core/NAPlayerState.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"

namespace
{
	const FLinearColor MenuRed(0.95f, 0.22f, 0.16f);
	const FLinearColor MenuText(0.92f, 0.92f, 0.92f);
	const FLinearColor MenuDim(0.6f, 0.6f, 0.62f);
	const FLinearColor MenuPanel(0.01f, 0.01f, 0.02f, 0.78f);
	const FLinearColor MenuEmber(1.f, 0.55f, 0.15f);

	const TCHAR* QualityName(int32 Level)
	{
		switch (Level)
		{
		case 0:  return TEXT("Low");
		case 1:  return TEXT("Medium");
		case 2:  return TEXT("High");
		case 3:  return TEXT("Epic");
		default: return TEXT("Custom");
		}
	}
}

FName ANAHUD::HitTestMenu(float X, float Y) const
{
	for (const FMenuButtonRect& Button : MenuButtons)
	{
		if (X >= Button.X && X <= Button.X + Button.W && Y >= Button.Y && Y <= Button.Y + Button.H)
		{
			return Button.Id;
		}
	}
	return NAME_None;
}

bool ANAHUD::MenuButton(FName Id, const FString& Label, float X, float Y, float W, float H, bool bEnabled, float TextScale)
{
	const bool bHover = bEnabled && MouseX >= X && MouseX <= X + W && MouseY >= Y && MouseY <= Y + H;
	DrawRect(bHover ? FLinearColor(MenuRed.R, MenuRed.G, MenuRed.B, 0.85f) : FLinearColor(0.f, 0.f, 0.f, 0.55f), X, Y, W, H);
	DrawRect(bHover ? FLinearColor::White : FLinearColor(MenuRed.R, MenuRed.G, MenuRed.B, bEnabled ? 0.9f : 0.3f), X, Y, 3.f, H);
	DrawText(Label, bEnabled ? (bHover ? FLinearColor::White : MenuText) : FLinearColor(0.45f, 0.45f, 0.45f), X + 14.f, Y + (H - 16.f * TextScale) * 0.5f, nullptr, TextScale);
	if (bEnabled)
	{
		MenuButtons.Add({ Id, X, Y, W, H });
	}
	return bHover;
}

void ANAHUD::MenuParagraph(const TArray<FString>& Lines, float X, float Y, float LineHeight, float TextScale)
{
	for (const FString& Line : Lines)
	{
		DrawText(Line, Line.StartsWith(TEXT("#")) ? MenuEmber : MenuText, X, Y, nullptr, TextScale);
		Y += LineHeight;
	}
}

void ANAHUD::DrawLogo(float X, float Y, float Scale)
{
	// NULL-ACTION wordmark: heavy letters, a red "null line" through the middle, a thin electric underline.
	UFont* Big = GEngine ? GEngine->GetLargeFont() : nullptr;
	const float T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	DrawText(TEXT("NULL"), MenuText, X, Y, Big, Scale);
	DrawText(TEXT("-"), MenuRed, X + 96.f * Scale, Y, Big, Scale);
	DrawText(TEXT("ACTION"), MenuRed, X + 112.f * Scale, Y, Big, Scale);
	const float Flicker = 0.75f + 0.25f * FMath::Sin(T * 7.f) * FMath::Sin(T * 2.3f);
	DrawRect(FLinearColor(MenuRed.R, MenuRed.G, MenuRed.B, Flicker), X, Y + 30.f * Scale, 260.f * Scale, 2.f);
	DrawRect(FLinearColor(0.4f, 0.8f, 1.f, 0.5f * Flicker), X, Y + 34.f * Scale, 120.f * Scale, 1.f);
}

void ANAHUD::DrawMenu(ANAPlayerController* PC, float ScreenW, float ScreenH)
{
	PC->GetMousePosition(MouseX, MouseY);
	const ENAMenuScreen Screen = PC->GetMenuScreen();
	const float CenterX = ScreenW * 0.5f;
	const ANAPlayerState* State = PC->GetPlayerState<ANAPlayerState>();
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();

	// ----- Intro: letterbox, captions, title reveal -----
	if (Screen == ENAMenuScreen::Intro)
	{
		const float Bar = ScreenH * 0.11f;
		DrawRect(FLinearColor::Black, 0.f, 0.f, ScreenW, Bar);
		DrawRect(FLinearColor::Black, 0.f, ScreenH - Bar, ScreenW, Bar);
		const float Alpha = PC->GetIntroCaptionAlpha();
		if (PC->IsIntroTitleShot())
		{
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.35f * Alpha), 0.f, 0.f, ScreenW, ScreenH);
			DrawLogo(CenterX - 190.f * 1.5f, ScreenH * 0.4f, 3.f * FMath::Max(0.6f, Alpha));
			DrawText(TEXT("Survive.  Hunt.  Rise from the ash."), FLinearColor(1.f, 1.f, 1.f, Alpha), CenterX - 150.f, ScreenH * 0.4f + 130.f, nullptr, 1.3f);
		}
		else
		{
			const FString Caption = PC->GetIntroCaption();
			DrawText(Caption, FLinearColor(1.f, 1.f, 1.f, Alpha), CenterX - Caption.Len() * 5.4f, ScreenH - Bar + Bar * 0.35f, nullptr, 1.35f);
		}
		DrawText(TEXT("SPACE / CLICK  skip"), FLinearColor(1.f, 1.f, 1.f, 0.4f), ScreenW - 190.f, ScreenH - 28.f, nullptr, 0.8f);
		return;
	}

	// Shared dark gradient on the left for readability over the live 3D scene.
	const bool bOverlay = Screen == ENAMenuScreen::Pause || ((Screen == ENAMenuScreen::Controls || Screen == ENAMenuScreen::Settings) && PC->GetViewTarget() == PC->GetPawn());
	if (bOverlay)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), 0.f, 0.f, ScreenW, ScreenH);
	}
	else
	{
		for (int32 Step = 0; Step < 8; ++Step)
		{
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.08f), 0.f, 0.f, ScreenW * (0.55f - Step * 0.05f), ScreenH);
		}
	}

	const float Left = 70.f;
	float Y = ScreenH * 0.14f;
	const float ButtonW = 330.f;
	const float ButtonH = 38.f;
	const float Gap = 46.f;
	auto Button = [&](FName Id, const FString& Label, bool bEnabled = true)
	{
		MenuButton(Id, Label, Left, Y, ButtonW, ButtonH, bEnabled);
		Y += Gap;
	};
	auto Heading = [&](const FString& Title)
	{
		DrawLogo(Left, ScreenH * 0.05f, 1.2f);
		DrawText(Title, MenuEmber, Left, ScreenH * 0.05f + 60.f, nullptr, 1.4f);
		Y = ScreenH * 0.05f + 110.f;
	};
	const float RightX = FMath::Max(Left + ButtonW + 60.f, ScreenW * 0.5f);
	const float RightW = FMath::Min(ScreenW - RightX - 50.f, 760.f);

	switch (Screen)
	{
	case ENAMenuScreen::Main:
	{
		DrawLogo(Left, ScreenH * 0.06f, 2.2f);
		DrawText(TEXT("Survive.  Hunt.  Rise from the ash."), MenuDim, Left, ScreenH * 0.06f + 92.f, nullptr, 1.1f);
		Y = ScreenH * 0.06f + 140.f;
		Button(TEXT("Play"), TEXT("PLAY"));
		Button(TEXT("Multiplayer"), TEXT("MULTIPLAYER"));
		Button(TEXT("Character"), TEXT("CHARACTER STUDIO"));
		Button(TEXT("Hunting"), TEXT("HUNTING GUIDE"));
		Button(TEXT("Missions"), TEXT("MISSIONS & EVENTS"));
		Button(TEXT("Inventory"), TEXT("INVENTORY"));
		Button(TEXT("Progression"), TEXT("PROGRESSION"));
		Button(TEXT("Friends"), TEXT("FRIENDS  (needs online accounts)"), false);
		Button(TEXT("Settings"), TEXT("SETTINGS"));
		Button(TEXT("News"), TEXT("NEWS & UPDATES"));
		Button(TEXT("Intro"), TEXT("REPLAY INTRO"));
		Button(TEXT("Exit"), TEXT("EXIT"));
		if (State)
		{
			DrawRect(MenuPanel, ScreenW - 360.f, ScreenH - 120.f, 320.f, 80.f);
			DrawText(State->GetPlayerName(), MenuText, ScreenW - 345.f, ScreenH - 110.f, nullptr, 1.1f);
			DrawText(FString::Printf(TEXT("Level %d   |   Ash %d/%d   |   %d kills"), State->GetPlayerLevel(), State->GetAshCharges(),
				ANAPlayerState::MaxAshCharges, State->GetKills()), MenuEmber, ScreenW - 345.f, ScreenH - 84.f, nullptr, 0.9f);
			DrawText(TEXT("dev build 2.1"), MenuDim, ScreenW - 345.f, ScreenH - 62.f, nullptr, 0.8f);
		}
		break;
	}
	case ENAMenuScreen::Multiplayer:
	{
		Heading(TEXT("MULTIPLAYER"));
		Button(TEXT("Host"), TEXT("HOST A GAME (listen server)"));
		Button(TEXT("JoinLocal"), TEXT("JOIN ON THIS PC (127.0.0.1)"));
		Button(TEXT("Back"), TEXT("BACK"));
		DrawRect(MenuPanel, RightX, ScreenH * 0.2f, RightW, 300.f);
		MenuParagraph({ TEXT("# Play with friends"),
			TEXT("Host: click HOST. Friends on your network join with ~ (console):  open <your IP>"),
			TEXT("Over the internet: the host forwards UDP port 7777, friends use the host's public IP."),
			TEXT("Dedicated server: Scripts/StartServer.bat, then Scripts/JoinServer.bat on each PC."),
			TEXT(""),
			TEXT("# Coming with online accounts (Stage 3B)"),
			TEXT("Server browser, friends list, invites and sign-in need the Epic Online Services"),
			TEXT("project to be set up by the owner first. Until then these stay disabled.") },
			RightX + 20.f, ScreenH * 0.2f + 20.f);
		break;
	}
	case ENAMenuScreen::Character:
	{
		Heading(TEXT("CHARACTER STUDIO"));
		const FNAAppearance& Look = PC->GetEditedAppearance();
		Button(TEXT("Outfit"), FString::Printf(TEXT("OUTFIT:  %s"), NAAppearance::OutfitName(Look.Outfit)));
		Button(TEXT("Skin"), FString::Printf(TEXT("SKIN:  %s"), NAAppearance::SkinName(Look.Skin)));
		Button(TEXT("Accent"), FString::Printf(TEXT("GOGGLES:  %s"), NAAppearance::AccentName(Look.Accent)));
		Button(TEXT("Helmet"), FString::Printf(TEXT("HELMET:  %s"), Look.bHelmet ? TEXT("On") : TEXT("Off")));
		Button(TEXT("Backpack"), FString::Printf(TEXT("BACKPACK:  %s"), Look.bBackpack ? TEXT("On") : TEXT("Off")));
		Y += 10.f;
		MenuButton(TEXT("RotL"), TEXT("< ROTATE"), Left, Y, 160.f, ButtonH);
		MenuButton(TEXT("RotR"), TEXT("ROTATE >"), Left + 170.f, Y, 160.f, ButtonH);
		Y += Gap;
		MenuButton(TEXT("ZoomIn"), TEXT("ZOOM +"), Left, Y, 160.f, ButtonH);
		MenuButton(TEXT("ZoomOut"), TEXT("ZOOM -"), Left + 170.f, Y, 160.f, ButtonH);
		Y += Gap + 10.f;
		Button(TEXT("SaveLook"), TEXT("SAVE LOOK"));
		Button(TEXT("ResetLook"), TEXT("RESET"));
		Button(TEXT("Back"), TEXT("BACK (discard)"));
		DrawText(TEXT("Your look is saved on this PC and shown to everyone on any server you join."), MenuDim, Left, Y + 10.f, nullptr, 0.8f);
		break;
	}
	case ENAMenuScreen::Hunting:
	{
		Heading(TEXT("HUNTING GUIDE"));
		Button(TEXT("Back"), TEXT("BACK"));
		DrawRect(MenuPanel, RightX, ScreenH * 0.12f, RightW, ScreenH * 0.76f);
		MenuParagraph({ TEXT("# ASH STALKER  -  den in the arena's far corner"),
			TEXT("Stalks, then ambushes when you look away. Bolder at night. Weak spot: glowing eyes."),
			TEXT("Drops hide, meat, Ember Fang (60%), Ember Cell (15%). Skin it with a Hunting Knife."),
			TEXT("# GREY WOLF  -  pack in the Whispering Pines (south-west)"),
			TEXT("Hunts in packs: one howls the others in. Drops pelts for Fur Coats."),
			TEXT("# DEER  -  near the pines"),
			TEXT("Skittish: crouch to approach. Meat and Deer Hide."),
			TEXT("# HORSE  -  herd west of the arena"),
			TEXT("Walk up slowly, feed Berries/Herbs 3x to tame. Ride, gallop, whistle (H)."),
			TEXT("# ASH GLIDER  -  roost on the Grey Peaks foothills (north-east)"),
			TEXT("Feed meat 3x while it's perched. A flying mount: storms make flight dangerous."),
			TEXT("# ASHEN BEHEMOTH (boss)  -  Ash Crater"),
			TEXT("Armored. Hit the ember core on its back or the head. Dodge the red circles. Bring friends."),
			TEXT("# GEAR"),
			TEXT("Bow (silent, +60% vs creatures), traps, bait, binoculars (B) to scan before you strike.") },
			RightX + 20.f, ScreenH * 0.12f + 20.f, 24.f, 0.85f);
		break;
	}
	case ENAMenuScreen::Missions:
	{
		Heading(TEXT("MISSIONS & WORLD EVENTS"));
		Button(TEXT("Back"), TEXT("BACK"));
		const ANAGameState* Game = GetWorld() ? GetWorld()->GetGameState<ANAGameState>() : nullptr;
		DrawRect(MenuPanel, RightX, ScreenH * 0.14f, RightW, ScreenH * 0.6f);
		TArray<FString> Lines;
		Lines.Add(TEXT("# HAPPENING NOW"));
		Lines.Add(Game && Game->IsEventActive() ? FString::Printf(TEXT("%s - %s"), *Game->GetEventTitle(), *Game->GetEventDetail()) : FString(TEXT("Nothing right now. Events start every 8-12 minutes.")));
		Lines.Add(TEXT(""));
		Lines.Add(TEXT("# EVENT TYPES (150 XP each)"));
		Lines.Add(TEXT("Creature Sighting - hunt the alpha Ash Stalker for a rare drop."));
		Lines.Add(TEXT("Supply Drop - first to open the crate wins Rare/Epic loot."));
		Lines.Add(TEXT("Distress Signal - switch on the beacon and hold the area for 45 s."));
		Lines.Add(TEXT("Ember Storm - survive the falling embers (red circles)."));
		Lines.Add(TEXT(""));
		Lines.Add(TEXT("# STANDING GOALS"));
		Lines.Add(TEXT("Slay the Ashen Behemoth (500 XP, Behemoth Heart). Loot the lake shipwreck."));
		Lines.Add(TEXT("Tame a horse and an Ash Glider. Attune to all three Ash Shrines."));
		MenuParagraph(Lines, RightX + 20.f, ScreenH * 0.14f + 20.f, 24.f, 0.85f);
		break;
	}
	case ENAMenuScreen::News:
	{
		Heading(TEXT("NEWS & UPDATES"));
		Button(TEXT("Back"), TEXT("BACK"));
		DrawRect(MenuPanel, RightX, ScreenH * 0.14f, RightW, ScreenH * 0.6f);
		MenuParagraph({ TEXT("# 2.1  Presentation"),
			TEXT("Main menu over the live world, intro, character studio, settings, pause menu."),
			TEXT("Plug in real 3D models: Project Settings > Game > NULL-ACTION Assets."),
			TEXT("# 2.0  The living world"),
			TEXT("Survival conditions, XP and skills, wolves and deer, world events, rarity loot,"),
			TEXT("shipwreck, binoculars, Engineer class, Ash Glider flying mount."),
			TEXT("# 1.x"),
			TEXT("Downed & revive, Ash Rebirth, class powers, hunting gear, the Ashen Behemoth, horses.") },
			RightX + 20.f, ScreenH * 0.14f + 20.f, 24.f, 0.9f);
		break;
	}
	case ENAMenuScreen::Settings:
	{
		Heading(TEXT("SETTINGS"));
		const int32 Quality = Settings ? Settings->GetOverallScalabilityLevel() : -1;
		const float Scale = Settings ? Settings->GetResolutionScaleNormalized() : 1.f;
		const float Fps = Settings ? Settings->GetFrameRateLimit() : 0.f;
		auto Row = [&](const FString& Label, const FString& Value, FName Minus, FName Plus)
		{
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), Left, Y, 560.f, ButtonH);
			DrawText(Label, MenuText, Left + 14.f, Y + 10.f, nullptr, 1.f);
			DrawText(Value, MenuEmber, Left + 260.f, Y + 10.f, nullptr, 1.f);
			if (Minus != NAME_None) { MenuButton(Minus, TEXT(" -"), Left + 420.f, Y + 4.f, 60.f, ButtonH - 8.f); }
			MenuButton(Plus, Minus != NAME_None ? TEXT(" +") : TEXT("toggle"), Left + 488.f, Y + 4.f, 66.f, ButtonH - 8.f);
			Y += Gap;
		};
		Row(TEXT("Graphics quality"), QualityName(Quality), TEXT("Quality-"), TEXT("Quality+"));
		Row(TEXT("Resolution scale"), FString::Printf(TEXT("%.0f%%"), 50.f + Scale * 50.f), TEXT("ResScale-"), TEXT("ResScale+"));
		Row(TEXT("Window mode"), Settings && Settings->GetFullscreenMode() != EWindowMode::Windowed ? TEXT("Fullscreen") : TEXT("Windowed"), NAME_None, TEXT("Fullscreen"));
		Row(TEXT("V-Sync"), Settings && Settings->IsVSyncEnabled() ? TEXT("On") : TEXT("Off"), NAME_None, TEXT("VSync"));
		Row(TEXT("Frame limit"), Fps <= 0.f ? FString(TEXT("Unlimited")) : FString::Printf(TEXT("%.0f"), Fps), NAME_None, TEXT("FpsLimit"));
		Row(TEXT("Mouse sensitivity"), FString::Printf(TEXT("%.1f"), PC->GetMouseSensitivity()), TEXT("Sens-"), TEXT("Sens+"));
		Row(TEXT("Field of view"), FString::Printf(TEXT("%.0f"), PC->GetFieldOfView()), TEXT("Fov-"), TEXT("Fov+"));
		Row(TEXT("Invert mouse Y"), PC->IsInvertY() ? TEXT("On") : TEXT("Off"), NAME_None, TEXT("InvertY"));
		Y += 10.f;
		Button(TEXT("Back"), TEXT("BACK"));
		DrawText(TEXT("Graphics apply immediately and are saved. Audio settings arrive with the sound pass."), MenuDim, Left, Y + 4.f, nullptr, 0.8f);
		break;
	}
	case ENAMenuScreen::Pause:
	{
		DrawLogo(CenterX - 160.f, ScreenH * 0.18f, 1.6f);
		DrawText(TEXT("PAUSED"), MenuEmber, CenterX - 40.f, ScreenH * 0.18f + 70.f, nullptr, 1.4f);
		float PauseY = ScreenH * 0.18f + 120.f;
		const float PauseX = CenterX - ButtonW * 0.5f;
		struct FPauseEntry { const TCHAR* Id; const TCHAR* Label; };
		const FPauseEntry Entries[] = { { TEXT("Resume"), TEXT("RESUME") }, { TEXT("Settings"), TEXT("SETTINGS") },
			{ TEXT("Controls"), TEXT("CONTROLS") }, { TEXT("MainMenu"), TEXT("MAIN MENU") }, { TEXT("Exit"), TEXT("QUIT GAME") } };
		for (const FPauseEntry& Entry : Entries)
		{
			MenuButton(FName(Entry.Id), Entry.Label, PauseX, PauseY, ButtonW, ButtonH);
			PauseY += Gap;
		}
		if (GetNetMode() != NM_Standalone)
		{
			DrawText(TEXT("Online: the world keeps going while you're paused."), MenuDim, PauseX, PauseY + 6.f, nullptr, 0.8f);
		}
		break;
	}
	case ENAMenuScreen::Controls:
	{
		Heading(TEXT("CONTROLS"));
		Button(TEXT("Back"), TEXT("BACK"));
		DrawRect(MenuPanel, RightX, ScreenH * 0.12f, RightW, ScreenH * 0.76f);
		MenuParagraph({ TEXT("# Move"),
			TEXT("WASD move   Mouse look   Shift sprint   Alt walk   C crouch / slide   Z prone   V dodge   Space jump / vault"),
			TEXT("# Fight"),
			TEXT("LMB fire   RMB aim   R reload   1-5 weapons   Q class power   B binoculars"),
			TEXT("# Survive"),
			TEXT("E interact / gather / revive (tap)   Crouch+E carry   I inventory & crafting   K skills"),
			TEXT("# Mounts"),
			TEXT("E ride   Shift gallop/boost   Space jump/take off/climb   C dive   E dismount/land   H whistle"),
			TEXT("# Other"),
			TEXT("Tab scoreboard   F1 controls overlay   Esc or P pause   ~ console (chat & commands)") },
			RightX + 20.f, ScreenH * 0.12f + 20.f, 26.f, 0.85f);
		break;
	}
	default:
		break;
	}
}
