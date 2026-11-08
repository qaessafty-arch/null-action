#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NATracer.generated.h"

class UStaticMeshComponent;

/** Short-lived, local-only bullet tracer plus impact marker. Cosmetic; never replicated. */
UCLASS(NotPlaceable)
class NULLACTION_API ANATracer : public AActor
{
	GENERATED_BODY()

public:
	ANATracer();

	static void Spawn(UWorld* World, const FVector& Start, const FVector& End);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|FX")
	TObjectPtr<UStaticMeshComponent> Beam;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|FX")
	TObjectPtr<UStaticMeshComponent> Impact;

private:
	void Setup(const FVector& Start, const FVector& End);
	void HideBeam();

	FTimerHandle BeamTimer;
};
