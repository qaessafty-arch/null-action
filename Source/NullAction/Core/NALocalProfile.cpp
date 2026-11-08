#include "Core/NALocalProfile.h"

#include "Kismet/GameplayStatics.h"

UNALocalProfile* UNALocalProfile::LoadOrCreate()
{
	if (UNALocalProfile* Loaded = Cast<UNALocalProfile>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
	{
		return Loaded;
	}
	return Cast<UNALocalProfile>(UGameplayStatics::CreateSaveGameObject(UNALocalProfile::StaticClass()));
}

void UNALocalProfile::Save()
{
	UGameplayStatics::SaveGameToSlot(this, SlotName, 0);
}
