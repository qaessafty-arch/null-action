#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NAHUD.generated.h"

/** Development HUD: controls reference and live movement/network state. Replaced by UMG UI later. */
UCLASS()
class NULLACTION_API ANAHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	void SetScoreboardVisible(bool bVisible) { bShowScoreboard = bVisible; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|HUD")
	bool bShowDebugOverlay = true;

private:
	void DrawChat(float ScreenH);
	void DrawKillFeed(float ScreenW);
	void DrawScoreboard(float ScreenW, float ScreenH);
	void DrawInventory(class ANAPlayerCharacter* Player, float ScreenW, float ScreenH);
	// NAHUD_Powers.cpp
	void DrawPowers(class ANAPlayerCharacter* Player, float ScreenW, float ScreenH);
	void DrawDeathScreen(float ScreenW, float ScreenH);
	bool ProjectToScreen(const FVector& WorldLocation, FVector2D& OutScreen) const;

	bool bShowScoreboard = false;

	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color, const FString& Label);
};
