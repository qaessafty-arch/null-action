#include "Weapons/NATracer.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ANATracer::ANATracer()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	InitialLifeSpan = 1.5f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	auto MakePart = [this, SceneRoot](const TCHAR* PartName, UStaticMesh* PartMesh)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
		Part->SetupAttachment(SceneRoot);
		Part->SetUsingAbsoluteLocation(true);
		Part->SetUsingAbsoluteRotation(true);
		Part->SetUsingAbsoluteScale(true);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->SetStaticMesh(PartMesh);
		if (ShapeMaterial.Succeeded())
		{
			Part->SetMaterial(0, ShapeMaterial.Object);
		}
		return Part;
	};
	Beam = MakePart(TEXT("Beam"), Cylinder.Object);
	Impact = MakePart(TEXT("Impact"), Cube.Object);
}

void ANATracer::Spawn(UWorld* World, const FVector& Start, const FVector& End)
{
	if (!World)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ANATracer* Tracer = World->SpawnActor<ANATracer>(ANATracer::StaticClass(), FTransform(Start), Params))
	{
		Tracer->Setup(Start, End);
	}
}

void ANATracer::BeginPlay()
{
	Super::BeginPlay();
	auto Tint = [](UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	};
	Tint(Beam, FLinearColor(1.f, 0.8f, 0.35f));
	Tint(Impact, FLinearColor(0.15f, 0.15f, 0.15f));
}

void ANATracer::Setup(const FVector& Start, const FVector& End)
{
	const FVector Delta = End - Start;
	const float Length = Delta.Size();

	// Engine cylinder: 100 units tall along Z, centred on its pivot.
	Beam->SetWorldLocationAndRotation((Start + End) * 0.5f, FRotationMatrix::MakeFromZ(Delta.GetSafeNormal()).Rotator());
	Beam->SetWorldScale3D(FVector(0.025f, 0.025f, FMath::Max(Length, 1.f) / 100.f));

	Impact->SetWorldLocationAndRotation(End, FRotator::ZeroRotator);
	Impact->SetWorldScale3D(FVector(0.05f));

	GetWorldTimerManager().SetTimer(BeamTimer, this, &ANATracer::HideBeam, 0.06f, false);
}

void ANATracer::HideBeam()
{
	Beam->SetVisibility(false);
}
