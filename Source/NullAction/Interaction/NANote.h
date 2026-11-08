#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NAInteractable.h"
#include "NANote.generated.h"

class UStaticMeshComponent;

/** A readable note / journal page for environmental storytelling. The server sends its text to the reader. */
UCLASS()
class NULLACTION_API ANANote : public AActor, public INAInteractable
{
	GENERATED_BODY()

public:
	ANANote();

	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual void Interact(ANACharacterBase* User) override;

	/** Server only; set before FinishSpawning. Each entry is shown as one line. */
	UPROPERTY(EditAnywhere, Category = "NullAction|Story") FString Title;
	UPROPERTY(EditAnywhere, Category = "NullAction|Story") TArray<FString> Lines;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Story") TObjectPtr<UStaticMeshComponent> Paper;
};
