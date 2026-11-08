#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NAPrecipitation.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Local-only (never replicated) rain, snow and blowing dust around the local camera, drawn with
 * instanced meshes so it needs no imported particle assets. The world environment sets the amounts.
 * Limitation: it doesn't yet stop at roofs (indoor occlusion comes with real buildings).
 */
UCLASS(NotPlaceable)
class NULLACTION_API ANAPrecipitation : public AActor
{
	GENERATED_BODY()

public:
	ANAPrecipitation();

	virtual void Tick(float DeltaSeconds) override;

	void SetIntensity(float NewRain, float NewSnow, float NewDust);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "NullAction|Weather") TObjectPtr<UInstancedStaticMeshComponent> RainDrops;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Weather") TObjectPtr<UInstancedStaticMeshComponent> SnowFlakes;
	UPROPERTY(VisibleAnywhere, Category = "NullAction|Weather") TObjectPtr<UInstancedStaticMeshComponent> DustMotes;

private:
	struct FLayer
	{
		TArray<FVector> Positions;
		TArray<FTransform> Transforms;
	};

	void InitLayer(FLayer& Layer, UInstancedStaticMeshComponent* Component, int32 Count, const FVector& Center);
	void UpdateLayer(FLayer& Layer, UInstancedStaticMeshComponent* Component, float Amount, const FVector& Center,
		const FVector& Velocity, const FVector& Scale, float DeltaSeconds, bool bWobble);

	FLayer Rain;
	FLayer Snow;
	FLayer Dust;
	float RainAmount = 0.f;
	float SnowAmount = 0.f;
	float DustAmount = 0.f;
	float Time = 0.f;
	bool bInitialised = false;
};
