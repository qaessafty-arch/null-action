#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NAHUD.generated.h"

/**
 * In-game HUD, drawn with Canvas (no widget assets needed): compass, vitals, weapon, prompts, powers,
 * boss bar, chat and kill feed. The full controls list is hidden behind F1. Moves to UMG in Stage 6.
 */
UCLASS()
class NULLACTION_API ANAHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	void SetScoreboardVisible(bool bVisible) { bShowScoreboard = bVisible; }
	void ToggleHelp() { bShowDebugOverlay = !bShowDebugOverlay; }

	/** Menu buttons drawn last frame: which one is under the mouse (NAME_None if none / disabled). */
	FName HitTestMenu(float X, float Y) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|HUD")
	bool bShowDebugOverlay = false;

private:
	void DrawChat(float ScreenH);
	void DrawKillFeed(float ScreenW);
	void DrawScoreboard(float ScreenW, float ScreenH);
	void DrawInventory(class ANAPlayerCharacter* Player, float ScreenW, float ScreenH);
	// NAHUD_Powers.cpp
	void DrawPowers(class ANAPlayerCharacter* Player, float ScreenW, float ScreenH);
	void DrawDeathScreen(float ScreenW, float ScreenH);
	void DrawMounted(class ANAHorse* Horse, float ScreenW, float ScreenH);
	void DrawProgressAndEvents(class ANAPlayerCharacter* Player, float ScreenW, float ScreenH);
	void DrawBinocularScan(class ANAPlayerCharacter* Player, float ScreenW, float ScreenH);
	// NAHUD_Menu.cpp
	void DrawMenu(class ANAPlayerController* PC, float ScreenW, float ScreenH);
	void DrawLogo(float X, float Y, float Scale);
	bool MenuButton(FName Id, const FString& Label, float X, float Y, float W, float H, bool bEnabled = true, float TextScale = 1.1f);
	void MenuParagraph(const TArray<FString>& Lines, float X, float Y, float LineHeight = 22.f, float TextScale = 0.9f);

	struct FMenuButtonRect { FName Id; float X = 0.f; float Y = 0.f; float W = 0.f; float H = 0.f; };
	TArray<FMenuButtonRect> MenuButtons;
	float MouseX = -1.f;
	float MouseY = -1.f;
	bool ProjectToScreen(const FVector& WorldLocation, FVector2D& OutScreen) const;

	bool bShowScoreboard = false;

	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color, const FString& Label);
	void DrawCompass(float CenterX, float Yaw);
	void DrawPanel(float X, float Y, float Width, float Height);
};
