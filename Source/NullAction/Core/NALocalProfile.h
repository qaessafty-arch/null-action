#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/NAPlayerState.h"
#include "NALocalProfile.generated.h"

/**
 * Settings and look saved on THIS computer (Saved/SaveGames/LocalProfile.sav): never sent anywhere except
 * the character look, which is sent to the server you join so other players can see it.
 */
UCLASS()
class NULLACTION_API UNALocalProfile : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr const TCHAR* SlotName = TEXT("LocalProfile");
	static UNALocalProfile* LoadOrCreate();
	void Save();

	UPROPERTY() int32 Version = 1;
	UPROPERTY() bool bSeenIntro = false;
	UPROPERTY() float MouseSensitivity = 1.f;
	UPROPERTY() float FieldOfView = 85.f;
	UPROPERTY() bool bInvertY = false;
	UPROPERTY() FNAAppearance Appearance;
};
