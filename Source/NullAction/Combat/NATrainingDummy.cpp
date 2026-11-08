#include "Combat/NATrainingDummy.h"

#include "Combat/NAHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ANATrainingDummy::ANATrainingDummy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AutoPossessAI = EAutoPossessAI::Disabled;
	GetCharacterMovement()->bRunPhysicsWithNoController = true; // Still has gravity without a controller.

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShapeMaterial.Succeeded())
	{
		FallbackBody->SetMaterial(0, ShapeMaterial.Object);
	}
}

void ANATrainingDummy::BeginPlay()
{
	Super::BeginPlay();

	// Orange body so dummies are easy to tell apart from players.
	if (UMaterialInstanceDynamic* MID = FallbackBody->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.85f, 0.35f, 0.05f));
	}
}

void ANATrainingDummy::OnKilled(AController* Killer)
{
	Super::OnKilled(Killer);
	GetWorldTimerManager().SetTimer(ReviveTimer, this, &ANATrainingDummy::ReviveDummy, ReviveDelaySeconds, false);
}

void ANATrainingDummy::ReviveDummy()
{
	if (HealthComponent)
	{
		HealthComponent->Revive(1.f);
	}
}
