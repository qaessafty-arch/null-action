#include "AI/NAEnemySoldier.h"

#include "AI/NAEnemyAIController.h"
#include "Character/NAPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Interaction/NAPickup.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/NAWeaponTypes.h"

ANAEnemySoldier::ANAEnemySoldier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = ANAEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	DefaultWeapons.Add(ANAWeapon_AssaultRifle::StaticClass());

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShapeMaterial.Succeeded())
	{
		FallbackBody->SetMaterial(0, ShapeMaterial.Object);
		FallbackVisor->SetMaterial(0, ShapeMaterial.Object);
	}
}

void ANAEnemySoldier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAEnemySoldier, AlertState);
	DOREPLIFETIME(ANAEnemySoldier, AlertTarget);
}

float ANAEnemySoldier::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Dealt = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// Getting shot reveals the shooter: fight back immediately.
	// (Only players count: a stray bullet from an ally must not start a fight between soldiers.)
	ANAPlayerCharacter* Shooter = EventInstigator ? Cast<ANAPlayerCharacter>(EventInstigator->GetPawn()) : nullptr;
	if (Dealt > 0.f && !IsDead() && Shooter)
	{
		if (ANAEnemyAIController* Brain = Cast<ANAEnemyAIController>(GetController()))
		{
			Brain->ReceiveAlert(Shooter, Shooter->GetActorLocation());
		}
	}
	return Dealt;
}

void ANAEnemySoldier::BeginPlay()
{
	Super::BeginPlay();

	// Dark red body with a glowing-red visor so enemies read clearly at a distance.
	if (UMaterialInstanceDynamic* BodyMID = FallbackBody->CreateAndSetMaterialInstanceDynamic(0))
	{
		BodyMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.25f, 0.03f, 0.03f));
	}
	if (UMaterialInstanceDynamic* VisorMID = FallbackVisor->CreateAndSetMaterialInstanceDynamic(0))
	{
		VisorMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.1f, 0.05f));
	}
}

void ANAEnemySoldier::SetAlert(ENAAlertState NewState, AActor* NewTarget)
{
	if (HasAuthority())
	{
		AlertState = NewState;
		AlertTarget = NewTarget;
	}
}

void ANAEnemySoldier::OnKilled(AController* Killer)
{
	SetAlert(ENAAlertState::Unaware, nullptr);

	// Drop an ammo box where the soldier fell (taken once, then disappears).
	if (HasAuthority())
	{
		const FTransform DropTransform(FRotator::ZeroRotator, GetActorLocation() - FVector(0.f, 0.f, 35.f));
		if (ANASupplyPickup* Drop = GetWorld()->SpawnActorDeferred<ANASupplyPickup>(ANASupplyPickup::StaticClass(), DropTransform,
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Drop->SupplyType = ENASupplyType::Ammo;
			Drop->Amount = 1.f;
			Drop->SetRespawnSeconds(0.f);
			Drop->SetLifeSpan(45.f);
			Drop->FinishSpawning(DropTransform);
		}
	}

	Super::OnKilled(Killer);
}
