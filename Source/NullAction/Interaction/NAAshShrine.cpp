#include "Interaction/NAAshShrine.h"

#include "Character/NAPlayerCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAPlayerController.h"
#include "Core/NAPlayerState.h"
#include "Engine/StaticMesh.h"
#include "Inventory/NAInventoryComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/NATracer.h"

ANAAshShrine::ANAAshShrine()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true; // Players need to see shrines from far away.

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Basic shapes are 100 cm across; a dark stone base, a 4 m pillar and a floating ember on top.
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetupAttachment(Root);
	Base->SetRelativeScale3D(FVector(2.2f, 2.2f, 0.3f));
	Base->SetRelativeLocation(FVector(0.f, 0.f, 15.f));

	Pillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pillar"));
	Pillar->SetupAttachment(Root);
	Pillar->SetRelativeScale3D(FVector(0.6f, 0.6f, 4.f));
	Pillar->SetRelativeLocation(FVector(0.f, 0.f, 230.f));

	Ember = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ember"));
	Ember->SetupAttachment(Root);
	Ember->SetRelativeScale3D(FVector(0.55f));
	Ember->SetRelativeLocation(FVector(0.f, 0.f, 480.f));
	Ember->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ember->CastShadow = false;

	if (Cylinder.Succeeded())
	{
		Base->SetStaticMesh(Cylinder.Object);
		Pillar->SetStaticMesh(Cylinder.Object);
	}
	if (Sphere.Succeeded()) { Ember->SetStaticMesh(Sphere.Object); }
	if (ShapeMaterial.Succeeded())
	{
		Base->SetMaterial(0, ShapeMaterial.Object);
		Pillar->SetMaterial(0, ShapeMaterial.Object);
		Ember->SetMaterial(0, ShapeMaterial.Object);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 490.f));
	Glow->SetLightColor(FLinearColor(1.f, 0.45f, 0.15f));
	Glow->SetIntensity(6000.f);
	Glow->SetAttenuationRadius(1400.f);
	Glow->SetCastShadows(false);
}

void ANAAshShrine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAAshShrine, ShrineName);
}

void ANAAshShrine::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInstanceDynamic* MID = Base->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.08f, 0.07f, 0.07f));
	}
	if (UMaterialInstanceDynamic* MID = Pillar->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.16f, 0.13f, 0.12f));
	}
	if (UMaterialInstanceDynamic* MID = Ember->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.35f, 0.05f));
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false); // The pulse is only visual.
	}
}

void ANAAshShrine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Now = GetWorld()->GetTimeSeconds();
	const float Pulse = 0.75f + 0.25f * FMath::Sin(Now * 2.2f);
	const float Flare = Now < FlareUntil ? 6.f : 1.f;
	Glow->SetIntensity(6000.f * Pulse * Flare);
	Ember->SetRelativeScale3D(FVector(0.5f + 0.06f * Pulse));
}

FTransform ANAAshShrine::GetRebirthTransform() const
{
	const FVector Location = GetActorLocation() + GetActorForwardVector() * 250.f + FVector(0.f, 0.f, 110.f);
	return FTransform(FRotator(0.f, GetActorRotation().Yaw + 180.f, 0.f), Location);
}

FText ANAAshShrine::GetInteractionPrompt(const ANACharacterBase* User) const
{
	const ANAPlayerState* State = User ? User->GetPlayerState<ANAPlayerState>() : nullptr;
	if (!State)
	{
		return FText::GetEmpty();
	}
	if (State->GetAttunedShrineName() != ShrineName)
	{
		return FText::FromString(FString::Printf(TEXT("Attune to %s (reborn here when you die)"), *ShrineName));
	}
	const ANAPlayerCharacter* Player = Cast<ANAPlayerCharacter>(User);
	const UNAInventoryComponent* Inventory = Player ? Player->GetInventoryComponent() : nullptr;
	if (State->GetAshCharges() >= ANAPlayerState::MaxAshCharges)
	{
		return FText::FromString(FString::Printf(TEXT("%s - attuned, Ash charges full (%d)"), *ShrineName, State->GetAshCharges()));
	}
	if (Inventory && Inventory->CountOf(EmberFangId()) > 0)
	{
		return FText::FromString(TEXT("Offer an Ember Fang (+1 Ash charge)"));
	}
	return FText::FromString(FString::Printf(TEXT("%s - attuned (%d charge%s). Ember Fangs from Ash Stalkers add charges."),
		*ShrineName, State->GetAshCharges(), State->GetAshCharges() == 1 ? TEXT("") : TEXT("s")));
}

bool ANAAshShrine::CanInteract(const ANACharacterBase* User) const
{
	return Cast<ANAPlayerCharacter>(User) != nullptr;
}

void ANAAshShrine::Interact(ANACharacterBase* User)
{
	ANAPlayerCharacter* Player = Cast<ANAPlayerCharacter>(User);
	ANAPlayerState* State = Player ? Player->GetPlayerState<ANAPlayerState>() : nullptr;
	ANAPlayerController* Controller = Player ? Cast<ANAPlayerController>(Player->GetController()) : nullptr;
	if (!HasAuthority() || !State)
	{
		return;
	}
	auto Tell = [Controller](const FString& Text)
	{
		if (Controller) { Controller->ClientReceiveChat(ENAChatChannel::System, FString(), Text); }
	};

	if (State->GetAttunedShrineName() != ShrineName || State->GetAttunedShrine() != this)
	{
		State->AttuneTo(this, ShrineName);
		Tell(FString::Printf(TEXT("You are attuned to %s. With an Ash charge, death brings you back here."), *ShrineName));
		return;
	}
	UNAInventoryComponent* Inventory = Player->GetInventoryComponent();
	if (State->GetAshCharges() < ANAPlayerState::MaxAshCharges && Inventory && Inventory->RemoveItem(EmberFangId(), 1))
	{
		State->SetAshCharges(State->GetAshCharges() + 1);
		Tell(FString::Printf(TEXT("The shrine accepts the Ember Fang. Ash charges: %d/%d"), State->GetAshCharges(), ANAPlayerState::MaxAshCharges));
		MulticastRebirthFlare();
	}
}

void ANAAshShrine::MulticastRebirthFlare_Implementation()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	FlareUntil = GetWorld()->GetTimeSeconds() + 1.5f;
	const FVector Top = GetActorLocation() + FVector(0.f, 0.f, 2500.f);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector Offset(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), 0.f);
		ANATracer::Spawn(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 480.f) + Offset, Top + Offset);
	}
}
