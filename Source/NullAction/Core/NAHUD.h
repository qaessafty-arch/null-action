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

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NullAction|HUD")
	bool bShowDebugOverlay = true;
};
