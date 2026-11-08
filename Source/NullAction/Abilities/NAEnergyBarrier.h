#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NAEnergyBarrier.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/**
 * Engineer's Energy Barrier: a 3 m x 2.2 m shield wall that blocks bullets, creatures and players for 10 s.
 * It has 300 health, so focused fire breaks it early (counterplay). Server-spawned and replicated.
 */
UCLASS()
class NULLACTION_API ANAEnergyBarrier : public AActor
{
	GENERATED_BODY()

public:
	ANAEnergyBarrier();
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Wall;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;
	UPROPERTY(Replicated) float Health = 300.f;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
