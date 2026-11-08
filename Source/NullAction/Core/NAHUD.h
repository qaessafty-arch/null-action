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
	bool ProjectToScreen(const FVector& WorldLocation, FVector2D& OutScreen) const;

	bool bShowScoreboard = false;

	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color, const FString& Label);
	void DrawCompass(float CenterX, float Yaw);
	void DrawPanel(float X, float Y, float Width, float Height);
};
