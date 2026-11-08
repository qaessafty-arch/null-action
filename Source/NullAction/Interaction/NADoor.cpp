#include "Interaction/NADoor.h"

#include "Character/NACharacterBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "NullActionDoor"

ANADoor::ANADoor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(SceneRoot);

	Panel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Panel"));
	Panel->SetupAttachment(Hinge);
	Panel->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName); // Blocks players, bullets and the look-trace.
	Handle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Handle"));
	Handle->SetupAttachment(Panel);
	Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Cube.Succeeded())
	{
		Panel->SetStaticMesh(Cube.Object);
		Handle->SetStaticMesh(Cube.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		Panel->SetMaterial(0, ShapeMaterial.Object);
		Handle->SetMaterial(0, ShapeMaterial.Object);
	}
}

void ANADoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANADoor, OpenDirection);
	DOREPLIFETIME(ANADoor, Width);
	DOREPLIFETIME(ANADoor, Height);
}

void ANADoor::BeginPlay()
{
	Super::BeginPlay();
	ApplySize();
	if (UMaterialInstanceDynamic* MID = Panel->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.11f, 0.05f));
	}
	if (UMaterialInstanceDynamic* HandleMID = Handle->CreateAndSetMaterialInstanceDynamic(0))
	{
		HandleMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.6f, 0.5f, 0.2f));
	}
}

void ANADoor::ApplySize()
{
	// Panel extends along +Y from the hinge; 5 cm thick.
	Panel->SetRelativeLocation(FVector(0.f, Width * 0.5f, Height * 0.5f));
	Panel->SetRelativeScale3D(FVector(0.05f, Width / 100.f, Height / 100.f));
	// Handle is in panel space (scaled), so express its size relative to the panel.
	Handle->SetRelativeLocation(FVector(0.f, 38.f, -2.f));
	Handle->SetRelativeScale3D(FVector(2.4f, 0.08f, 0.02f));
}

FText ANADoor::GetInteractionPrompt(const ANACharacterBase* User) const
{
	return OpenDirection == 0 ? LOCTEXT("Open", "Open door") : LOCTEXT("Close", "Close door");
}

void ANADoor::Interact(ANACharacterBase* User)
{
	if (!HasAuthority() || !User)
	{
		return;
	}
	if (OpenDirection != 0)
	{
		OpenDirection = 0;
		return;
	}
	// Swing away from the person opening it.
	const FVector Local = GetActorTransform().InverseTransformPosition(User->GetActorLocation());
	OpenDirection = Local.X >= 0.f ? -1 : 1;
}

void ANADoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float TargetYaw = 95.f * OpenDirection;
	if (!FMath::IsNearlyEqual(CurrentYaw, TargetYaw, 0.1f))
	{
		CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaSeconds, 6.f);
		Hinge->SetRelativeRotation(FRotator(0.f, CurrentYaw, 0.f));
	}
}

#undef LOCTEXT_NAMESPACE
