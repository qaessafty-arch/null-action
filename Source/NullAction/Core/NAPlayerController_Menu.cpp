// Front end: intro, main menu over the live world, character studio, settings and the pause menu.
#include "Core/NAPlayerController.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Character/NAPlayerCharacter.h"
#include "Core/NAHUD.h"
#include "Core/NALocalProfile.h"
#include "Core/NAPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "World/NATerrainManager.h"
#include "Components/AudioComponent.h"
#include "Core/NAAssetSettings.h"

void ANAPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}
	Profile = UNALocalProfile::LoadOrCreate();
	if (Profile)
	{
		MouseSensitivity = Profile->MouseSensitivity;
		FieldOfView = Profile->FieldOfView;
		bInvertY = Profile->bInvertY;
		EditedAppearance = Profile->Appearance;
		ServerSetAppearance(Profile->Appearance); // Your saved look follows you to every server.
	}
	// First launch: the intro. Afterwards: straight to the main menu.
	OpenMenu(Profile && Profile->bSeenIntro ? ENAMenuScreen::Main : ENAMenuScreen::Intro);
}

void ANAPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent)
	{
		return;
	}
	// These never swallow the key, so gameplay input still works when no menu is open.
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ANAPlayerController::TogglePauseMenu).bConsumeInput = false;
	InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ANAPlayerController::TogglePauseMenu).bConsumeInput = false;
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ANAPlayerController::OnMenuClick).bConsumeInput = false;
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ANAPlayerController::SkipIntro).bConsumeInput = false;
}

void ANAPlayerController::SaveProfile()
{
	if (!Profile)
	{
		Profile = UNALocalProfile::LoadOrCreate();
	}
	if (Profile)
	{
		Profile->MouseSensitivity = MouseSensitivity;
		Profile->FieldOfView = FieldOfView;
		Profile->bInvertY = bInvertY;
		Profile->Save();
	}
}

void ANAPlayerController::ServerSetInMenu_Implementation(bool bInMenu)
{
	if (ANAPlayerCharacter* Body = Cast<ANAPlayerCharacter>(GetPawn()))
	{
		Body->SetInMenu(bInMenu);
	}
}

void ANAPlayerController::ServerSetAppearance_Implementation(const FNAAppearance& Look)
{
	if (ANAPlayerState* State = GetPlayerState<ANAPlayerState>())
	{
		State->SetAppearance(Look); // Indices are clamped on the server.
	}
}

void ANAPlayerController::OpenMenu(ENAMenuScreen Screen)
{
	if (!IsLocalController())
	{
		return;
	}
	const bool bWasOpen = IsMenuOpen();
	const bool bFromGameplayView = !bWasOpen || GetViewTarget() != MenuCamera;
	MenuScreen = Screen;
	if (ANAPlayerCharacter* PlayerBody = Cast<ANAPlayerCharacter>(GetPawn()))
	{
		PlayerBody->CloseInventoryMenu();
	}

	// Pause (and settings/controls opened from pause) stay over the gameplay view.
	const bool bOverlay = Screen == ENAMenuScreen::Pause || ((Screen == ENAMenuScreen::Controls || Screen == ENAMenuScreen::Settings) && bWasOpen && bFromGameplayView);
	if (bOverlay)
	{
		// Pause keeps the gameplay camera; single player actually pauses.
		if (GetNetMode() == NM_Standalone) { UGameplayStatics::SetGamePaused(this, true); }
	}
	else
	{
		if (!MenuCamera)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			MenuCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform(FVector(0.f, -3500.f, 1500.f)), Params);
			if (MenuCamera && MenuCamera->GetCameraComponent())
			{
				MenuCamera->GetCameraComponent()->SetFieldOfView(70.f);
				MenuCamera->GetCameraComponent()->bConstrainAspectRatio = false;
			}
		}
		if (MenuCamera && GetViewTarget() != MenuCamera)
		{
			SetViewTargetWithBlend(MenuCamera, bWasOpen ? 0.5f : 0.f);
		}
		if (Screen == ENAMenuScreen::Intro)
		{
			BuildIntroShots();
		}
	}

	SyncMenuPawnState();
	// Menu music plays under every menu except the in-game pause overlay.
	if (!MenuMusic)
	{
		if (USoundBase* Music = NAAssets::LoadSound(UNAAssetSettings::Get()->MenuMusic))
		{
			MenuMusic = UGameplayStatics::CreateSound2D(this, Music, 0.8f, 1.f, 0.f, nullptr, false, false);
		}
	}
	if (MenuMusic && !bOverlay && !MenuMusic->IsPlaying())
	{
		MenuMusic->FadeIn(1.5f, 0.8f);
	}
	const bool bCursor = Screen != ENAMenuScreen::Intro;
	bShowMouseCursor = bCursor;
	if (bCursor)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}

void ANAPlayerController::CloseMenu()
{
	if (!IsMenuOpen())
	{
		return;
	}
	MenuScreen = ENAMenuScreen::None;
	if (MenuMusic && MenuMusic->IsPlaying()) { MenuMusic->FadeOut(1.5f, 0.f); }
	if (GetNetMode() == NM_Standalone) { UGameplayStatics::SetGamePaused(this, false); }
	if (APawn* Body = GetPawn())
	{
		SetViewTargetWithBlend(Body, 0.6f);
		Body->EnableInput(this);
	}
	MenuDisabledPawn = nullptr;
	if (bMenuProtectionSent)
	{
		ServerSetInMenu(false);
		bMenuProtectionSent = false;
	}
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

bool ANAPlayerController::UsesMenuCamera() const
{
	return IsMenuOpen() && MenuCamera && GetViewTarget() == MenuCamera;
}

void ANAPlayerController::SyncMenuPawnState()
{
	APawn* Body = GetPawn();
	if (!IsMenuOpen() || !Body)
	{
		return;
	}
	if (MenuDisabledPawn.Get() != Body)
	{
		Body->DisableInput(this); // No walking or shooting behind the menu.
		MenuDisabledPawn = Body;
	}
	// Full-screen menus (with the menu camera) protect and hide your body; the pause overlay doesn't,
	// so pausing can't be used to dodge a fight online.
	const bool bProtect = UsesMenuCamera() || MenuScreen == ENAMenuScreen::Intro;
	if (bProtect != bMenuProtectionSent)
	{
		ServerSetInMenu(bProtect);
		bMenuProtectionSent = bProtect;
	}
}

void ANAPlayerController::TogglePauseMenu()
{
	if (!IsLocalController())
	{
		return;
	}
	switch (MenuScreen)
	{
	case ENAMenuScreen::None:     OpenMenu(ENAMenuScreen::Pause); break;
	case ENAMenuScreen::Intro:    SkipIntro(); break;
	case ENAMenuScreen::Pause:    CloseMenu(); break;
	case ENAMenuScreen::Main:     break; // Use Play or Exit.
	case ENAMenuScreen::Controls:
	case ENAMenuScreen::Settings: OpenMenu(GetViewTarget() == MenuCamera ? ENAMenuScreen::Main : ENAMenuScreen::Pause); break;
	default:                      OpenMenu(ENAMenuScreen::Main); break;
	}
}

void ANAPlayerController::OnMenuClick()
{
	if (!IsMenuOpen())
	{
		return;
	}
	if (MenuScreen == ENAMenuScreen::Intro)
	{
		SkipIntro();
		return;
	}
	float MouseX = 0.f;
	float MouseY = 0.f;
	ANAHUD* GameHUD = Cast<ANAHUD>(GetHUD());
	if (GameHUD && GetMousePosition(MouseX, MouseY))
	{
		const FName Action = GameHUD->HitTestMenu(MouseX, MouseY);
		if (Action != NAME_None)
		{
			if (USoundBase* Click = NAAssets::LoadSound(UNAAssetSettings::Get()->UIClick))
			{
				UGameplayStatics::PlaySound2D(this, Click);
			}
			HandleMenuAction(Action);
		}
	}
}

void ANAPlayerController::SkipIntro()
{
	if (MenuScreen != ENAMenuScreen::Intro)
	{
		return;
	}
	if (Profile)
	{
		Profile->bSeenIntro = true;
		Profile->Save();
	}
	OpenMenu(ENAMenuScreen::Main);
}

void ANAPlayerController::ApplyGraphicsDelta(int32 QualityDelta)
{
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		const int32 Current = FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, 3);
		Settings->SetOverallScalabilityLevel(FMath::Clamp(Current + QualityDelta, 0, 3));
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}
}

void ANAPlayerController::HandleMenuAction(FName Action)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	ANAPlayerCharacter* PlayerBody = Cast<ANAPlayerCharacter>(GetPawn());
	const FString Id = Action.ToString();

	// ----- Navigation -----
	if (Id == TEXT("Play") || Id == TEXT("Resume"))   { CloseMenu(); return; }
	if (Id == TEXT("Back"))                           { OpenMenu(GetViewTarget() == MenuCamera ? ENAMenuScreen::Main : ENAMenuScreen::Pause); return; }
	if (Id == TEXT("MainMenu"))                       { OpenMenu(ENAMenuScreen::Main); return; }
	if (Id == TEXT("Multiplayer"))                    { OpenMenu(ENAMenuScreen::Multiplayer); return; }
	if (Id == TEXT("Character"))                      { EditedAppearance = Profile ? Profile->Appearance : FNAAppearance(); OpenMenu(ENAMenuScreen::Character); return; }
	if (Id == TEXT("Hunting"))                        { OpenMenu(ENAMenuScreen::Hunting); return; }
	if (Id == TEXT("Missions"))                       { OpenMenu(ENAMenuScreen::Missions); return; }
	if (Id == TEXT("Settings"))                       { OpenMenu(ENAMenuScreen::Settings); return; }
	if (Id == TEXT("News"))                           { OpenMenu(ENAMenuScreen::News); return; }
	if (Id == TEXT("Controls"))                       { OpenMenu(ENAMenuScreen::Controls); return; }
	if (Id == TEXT("Intro"))                          { OpenMenu(ENAMenuScreen::Intro); return; }
	if (Id == TEXT("Inventory") || Id == TEXT("Progression"))
	{
		CloseMenu();
		if (PlayerBody) { PlayerBody->OpenInventoryTab(Id == TEXT("Inventory") ? 0 : 2); }
		return;
	}
	if (Id == TEXT("Exit"))                           { ConsoleCommand(TEXT("quit")); return; }

	// ----- Multiplayer (real engine travel; see Docs/MULTIPLAYER.md for internet play) -----
	if (Id == TEXT("Host"))      { ConsoleCommand(TEXT("open /Game/NullAction/Maps/L_TestArena?listen")); return; }
	if (Id == TEXT("JoinLocal")) { ConsoleCommand(TEXT("open 127.0.0.1")); return; }

	// ----- Settings -----
	if (Settings)
	{
		bool bApply = true;
		if (Id == TEXT("Quality+"))        { ApplyGraphicsDelta(+1); bApply = false; }
		else if (Id == TEXT("Quality-"))   { ApplyGraphicsDelta(-1); bApply = false; }
		else if (Id == TEXT("Fullscreen")) { Settings->SetFullscreenMode(Settings->GetFullscreenMode() == EWindowMode::Windowed ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed); }
		else if (Id == TEXT("VSync"))      { Settings->SetVSyncEnabled(!Settings->IsVSyncEnabled()); }
		else if (Id == TEXT("FpsLimit"))
		{
			const float Limit = Settings->GetFrameRateLimit();
			Settings->SetFrameRateLimit(Limit <= 0.f ? 60.f : Limit < 61.f ? 120.f : Limit < 121.f ? 144.f : 0.f);
		}
		else if (Id == TEXT("ResScale+"))  { Settings->SetResolutionScaleNormalized(FMath::Clamp(Settings->GetResolutionScaleNormalized() + 0.1f, 0.f, 1.f)); }
		else if (Id == TEXT("ResScale-"))  { Settings->SetResolutionScaleNormalized(FMath::Clamp(Settings->GetResolutionScaleNormalized() - 0.1f, 0.f, 1.f)); }
		else { bApply = false; }
		if (bApply)
		{
			Settings->ApplySettings(false);
			Settings->SaveSettings();
			return;
		}
	}
	if (Id == TEXT("Sens+"))   { MouseSensitivity = FMath::Clamp(MouseSensitivity + 0.1f, 0.2f, 3.f); SaveProfile(); return; }
	if (Id == TEXT("Sens-"))   { MouseSensitivity = FMath::Clamp(MouseSensitivity - 0.1f, 0.2f, 3.f); SaveProfile(); return; }
	if (Id == TEXT("Fov+"))    { FieldOfView = FMath::Clamp(FieldOfView + 5.f, 70.f, 110.f); SaveProfile(); return; }
	if (Id == TEXT("Fov-"))    { FieldOfView = FMath::Clamp(FieldOfView - 5.f, 70.f, 110.f); SaveProfile(); return; }
	if (Id == TEXT("InvertY")) { bInvertY = !bInvertY; SaveProfile(); return; }

	// ----- Character studio: edits preview live on your own body; Save keeps them -----
	bool bLookChanged = true;
	if (Id == TEXT("Outfit"))        { EditedAppearance.Outfit = static_cast<uint8>((EditedAppearance.Outfit + 1) % NAAppearance::OutfitCount); }
	else if (Id == TEXT("Skin"))     { EditedAppearance.Skin = static_cast<uint8>((EditedAppearance.Skin + 1) % NAAppearance::SkinCount); }
	else if (Id == TEXT("Accent"))   { EditedAppearance.Accent = static_cast<uint8>((EditedAppearance.Accent + 1) % NAAppearance::AccentCount); }
	else if (Id == TEXT("Helmet"))   { EditedAppearance.bHelmet = !EditedAppearance.bHelmet; }
	else if (Id == TEXT("Backpack")) { EditedAppearance.bBackpack = !EditedAppearance.bBackpack; }
	else if (Id == TEXT("ResetLook")) { EditedAppearance = FNAAppearance(); }
	else { bLookChanged = false; }
	if (bLookChanged)
	{
		if (PlayerBody) { PlayerBody->ApplyAppearance(EditedAppearance); }
		return;
	}
	if (Id == TEXT("SaveLook"))
	{
		if (Profile)
		{
			Profile->Appearance = EditedAppearance;
			Profile->Save();
		}
		ServerSetAppearance(EditedAppearance);
		OpenMenu(ENAMenuScreen::Main);
		return;
	}
	if (Id == TEXT("RotL"))    { StudioYaw -= 30.f; return; }
	if (Id == TEXT("RotR"))    { StudioYaw += 30.f; return; }
	if (Id == TEXT("ZoomIn"))  { StudioDistance = FMath::Clamp(StudioDistance - 40.f, 120.f, 500.f); return; }
	if (Id == TEXT("ZoomOut")) { StudioDistance = FMath::Clamp(StudioDistance + 40.f, 120.f, 500.f); return; }
}

void ANAPlayerController::BuildIntroShots()
{
	// A camera journey through the real, live world (not a pre-rendered video): the story in five shots.
	IntroShots.Reset();
	const FVector2D Lake = NATerrain::LakeCenter();
	const FVector2D Crater = NATerrain::CraterCenter();
	const FVector2D Cabin = NATerrain::CabinCenter();
	auto Ground = [](float X, float Y) { return FVector(X, Y, FMath::Max(NATerrain::HeightAt(X, Y), NATerrain::WaterLevel())); };
	IntroShots.Add({ Ground(Lake.X + 6000.f, Lake.Y - 9000.f) + FVector(0.f, 0.f, 2600.f), Ground(Lake.X + 3000.f, Lake.Y - 6000.f) + FVector(0.f, 0.f, 1400.f),
		Ground(Lake.X, Lake.Y), 6.f, TEXT("SITE NULL. The Array ran its final cycle at 03:17."), false });
	IntroShots.Add({ Ground(Crater.X - 4500.f, Crater.Y - 3500.f) + FVector(0.f, 0.f, 1800.f), Ground(Crater.X - 2800.f, Crater.Y - 2000.f) + FVector(0.f, 0.f, 900.f),
		Ground(Crater.X, Crater.Y) + FVector(0.f, 0.f, 200.f), 6.f, TEXT("Something answered. The sky turned to ash, and the ground split open."), false });
	IntroShots.Add({ Ground(Cabin.X + 1800.f, Cabin.Y + 1600.f) + FVector(0.f, 0.f, 500.f), Ground(Cabin.X + 900.f, Cabin.Y + 900.f) + FVector(0.f, 0.f, 260.f),
		Ground(Cabin.X, Cabin.Y) + FVector(0.f, 0.f, 150.f), 6.f, TEXT("The few who stayed learned to hunt what came out of the dark."), false });
	IntroShots.Add({ FVector(1800.f, -4200.f, 700.f), FVector(900.f, -3300.f, 450.f), FVector(300.f, -2600.f, 400.f), 6.f,
		TEXT("And some of the dead... came back. They call it the Ash Rebirth."), false });
	IntroShots.Add({ FVector(-6000.f, -6000.f, 4000.f), FVector(-4500.f, -4500.f, 3200.f), FVector(0.f, 0.f, 300.f), 5.f, FString(), true });
	IntroShotIndex = 0;
	IntroShotTime = 0.f;
}

void ANAPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (IsLocalController() && IsMenuOpen())
	{
		// Possession can happen after the menu opened (start of play, respawn): keep camera and input in sync.
		const bool bOverlayScreen = MenuScreen == ENAMenuScreen::Pause || MenuScreen == ENAMenuScreen::Controls ||
			(MenuScreen == ENAMenuScreen::Settings && GetViewTarget() != MenuCamera);
		if (!bOverlayScreen && MenuCamera && GetViewTarget() != MenuCamera)
		{
			SetViewTarget(MenuCamera);
		}
		SyncMenuPawnState();
		UpdateMenuCamera(DeltaTime);
	}
}

void ANAPlayerController::UpdateMenuCamera(float DeltaTime)
{
	if (!MenuCamera || MenuScreen == ENAMenuScreen::Pause || MenuScreen == ENAMenuScreen::Controls)
	{
		return;
	}
	FVector Location;
	FVector LookAt;

	if (MenuScreen == ENAMenuScreen::Intro && IntroShots.IsValidIndex(IntroShotIndex))
	{
		const FIntroShot& Shot = IntroShots[IntroShotIndex];
		IntroShotTime += DeltaTime;
		const float Alpha = FMath::Clamp(IntroShotTime / Shot.Duration, 0.f, 1.f);
		const float Smooth = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
		Location = FMath::Lerp(Shot.From, Shot.To, Smooth);
		LookAt = Shot.LookAt;
		IntroCaption = Shot.Caption;
		bIntroTitle = Shot.bTitle;
		IntroCaptionAlpha = FMath::Clamp(FMath::Min(IntroShotTime / 0.8f, (Shot.Duration - IntroShotTime) / 0.8f), 0.f, 1.f);
		if (IntroShotTime >= Shot.Duration)
		{
			++IntroShotIndex;
			IntroShotTime = 0.f;
			if (!IntroShots.IsValidIndex(IntroShotIndex))
			{
				SkipIntro(); // Finished: remember it and go to the main menu.
				return;
			}
		}
	}
	else if (MenuScreen == ENAMenuScreen::Character && GetPawn())
	{
		// Character studio: orbit your own body.
		const FVector Center = GetPawn()->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		const FVector Offset = FRotator(-8.f, GetPawn()->GetActorRotation().Yaw + StudioYaw, 0.f).Vector() * StudioDistance;
		Location = Center + Offset;
		LookAt = Center + FVector(0.f, 0.f, 10.f);
		Location = FMath::VInterpTo(MenuCamera->GetActorLocation(), Location, DeltaTime, 6.f);
	}
	else
	{
		// Main menu backdrop: a slow aerial orbit over the arena while the world keeps living.
		MenuOrbitYaw = FMath::Fmod(MenuOrbitYaw + DeltaTime * 3.f, 360.f);
		LookAt = FVector(0.f, 0.f, 300.f);
		Location = LookAt + FRotator(-14.f, MenuOrbitYaw, 0.f).Vector() * -5200.f + FVector(0.f, 0.f, 900.f);
	}
	MenuCamera->SetActorLocationAndRotation(Location, (LookAt - Location).Rotation());
}
