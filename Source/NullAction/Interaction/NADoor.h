#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NAInteractable.h"
#include "NADoor.generated.h"

class UStaticMeshComponent;

/** Hinged door. The actor sits on the hinge; the panel swings away from whoever opens it. Replicated. */
UCLASS()
class NULLACTION_API ANADoor : public AActor, public INAInteractable
{
	GENERATED_BODY()

public:
	ANADoor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual void Interact(ANACharacterBase* User) override;

	/** Width/height in cm; set before FinishSpawning. */
	UPROPERTY(EditAnywhere, Replicated, Category = "NullAction|Door") float Width = 110.f;
	UPROPERTY(EditAnywhere, Replicated, Category = "NullAction|Door") float Height = 220.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Door") TObjectPtr<USceneComponent> Hinge;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Door") TObjectPtr<UStaticMeshComponent> Panel;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Door") TObjectPtr<UStaticMeshComponent> Handle;

	/** 0 = closed, +1/-1 = open toward either side. */
	UPROPERTY(Replicated) int8 OpenDirection = 0;

private:
	void ApplySize();
	float CurrentYaw = 0.f;
};
