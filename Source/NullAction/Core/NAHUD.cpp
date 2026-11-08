#include "Core/NAHUD.h"

#include "Character/NACharacterBase.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"

namespace
{
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

void ANAHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!bShowDebugOverlay || !Canvas)
	{
		return;
	}

	const FLinearColor Title(0.95f, 0.25f, 0.2f);
	const FLinearColor Body(0.9f, 0.9f, 0.9f);
	float Y = 24.f;
	auto Line = [this, &Y](const FString& Text, const FLinearColor& Color)
	{
		DrawText(Text, Color, 24.f, Y, nullptr, 1.f);
		Y += 18.f;
	};

	Line(TEXT("NULL-ACTION  //  dev build 0.1"), Title);
	Line(TEXT("WASD / Left stick: move     Mouse / Right stick: look"), Body);
	Line(TEXT("Shift: sprint (hold)   Alt: walk (toggle)   C / Ctrl: crouch   Space: jump"), Body);

	const APlayerController* PC = GetOwningPlayerController();
	const ANACharacterBase* Character = PC ? Cast<ANACharacterBase>(PC->GetPawn()) : nullptr;
	if (Character)
	{
		Y += 8.f;
		Line(FString::Printf(TEXT("Gait: %s%s   Speed: %.0f cm/s   %s"),
			GaitName(Character->GetGait()),
			Character->bIsCrouched ? TEXT(" (crouched)") : TEXT(""),
			Character->GetVelocity().Size2D(),
			NetModeName(GetNetMode())), Body);
	}
}
