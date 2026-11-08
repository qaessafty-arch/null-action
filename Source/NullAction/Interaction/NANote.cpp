#include "Interaction/NANote.h"

#include "Character/NACharacterBase.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAPlayerController.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "NullActionNote"

ANANote::ANANote()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Paper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Paper"));
	RootComponent = Paper;
	Paper->SetRelativeScale3D(FVector(0.3f, 0.22f, 0.01f));
	// Only the look-trace needs to hit it.
	Paper->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Paper->SetCollisionResponseToAllChannels(ECR_Ignore);
	Paper->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	if (Cube.Succeeded()) { Paper->SetStaticMesh(Cube.Object); }
	if (ShapeMaterial.Succeeded()) { Paper->SetMaterial(0, ShapeMaterial.Object); }
}

void ANANote::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInstanceDynamic* MID = Paper->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.85f, 0.8f, 0.65f));
	}
}

FText ANANote::GetInteractionPrompt(const ANACharacterBase* User) const
{
	return LOCTEXT("Read", "Read note");
}

void ANANote::Interact(ANACharacterBase* User)
{
	ANAPlayerController* Reader = User ? Cast<ANAPlayerController>(User->GetController()) : nullptr;
	if (!HasAuthority() || !Reader)
	{
		return;
	}
	Reader->ClientReceiveChat(ENAChatChannel::System, FString(), FString::Printf(TEXT("--- %s ---"), *Title));
	for (const FString& Line : Lines)
	{
		Reader->ClientReceiveChat(ENAChatChannel::System, FString(), Line);
	}
}

#undef LOCTEXT_NAMESPACE
