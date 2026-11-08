#include "Abilities/NAEnergyBarrier.h"

#include "NullAction.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ANAEnergyBarrier::ANAEnergyBarrier()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	InitialLifeSpan = 10.f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Wall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall"));
	RootComponent = Wall;
	if (Cube.Succeeded()) { Wall->SetStaticMesh(Cube.Object); }
	if (ShapeMaterial.Succeeded()) { Wall->SetMaterial(0, ShapeMaterial.Object); }
	Wall->SetRelativeScale3D(FVector(0.15f, 3.f, 2.2f));
	Wall->SetCollisionProfileName(TEXT("BlockAll"));
	Wall->SetCollisionResponseToChannel(NA_TRACE_WEAPON, ECR_Block);
	Wall->SetCanEverAffectNavigation(false);
	Wall->CastShadow = false;

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Wall);
	Glow->SetLightColor(FLinearColor(0.2f, 0.6f, 1.f));
	Glow->SetIntensity(5000.f);
	Glow->SetAttenuationRadius(600.f);
	Glow->SetCastShadows(false);
}

void ANAEnergyBarrier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAEnergyBarrier, Health);
}

void ANAEnergyBarrier::BeginPlay()
{
	Super::BeginPlay();
	// Lift so the bottom edge sits on the ground (cube is 100 units, centred).
	AddActorWorldOffset(FVector(0.f, 0.f, 20.f));
	if (UMaterialInstanceDynamic* MID = Wall->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.15f, 0.5f, 1.f));
	}
}

float ANAEnergyBarrier::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (HasAuthority() && Damage > 0.f)
	{
		Health -= Damage;
		if (Health <= 0.f)
		{
			Destroy(); // Focused fire breaks it.
		}
	}
	return Damage;
}

void ANAEnergyBarrier::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Flickers as it weakens or runs out.
	const float Life = GetLifeSpan();
	const float Strength = FMath::Clamp(Health / 300.f, 0.2f, 1.f) * (Life > 0.f && Life < 2.f ? (FMath::Sin(GetWorld()->GetTimeSeconds() * 30.f) > 0.f ? 1.f : 0.3f) : 1.f);
	Glow->SetIntensity(5000.f * Strength);
}
