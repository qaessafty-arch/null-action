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

	// Orange so dummies are easy to tell apart from players.
	FallbackOutfitColor = FLinearColor(0.85f, 0.35f, 0.05f);
	FallbackPantsColor = FLinearColor(0.6f, 0.25f, 0.04f);
	FallbackSkinColor = FLinearColor(0.75f, 0.65f, 0.5f);
	FallbackAccentColor = FLinearColor(0.1f, 0.1f, 0.1f);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShapeMaterial.Succeeded())
	{
		FallbackBody->SetMaterial(0, ShapeMaterial.Object);
	}
}

void ANATrainingDummy::BeginPlay()
{
	Super::BeginPlay();
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
