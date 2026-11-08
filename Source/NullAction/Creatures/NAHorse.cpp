#include "Creatures/NAHorse.h"

#include "AIController.h"
#include "Camera/CameraComponent.h"
#include "Character/NAPlayerCharacter.h"
#include "Combat/NAHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAGameMode.h"
#include "Core/NAPlayerController.h"
#include "Core/NAPlayerState.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Interaction/NAResourceNode.h"
#include "Inventory/NAInventoryComponent.h"
#include "Inventory/NAItems.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "World/NAWaterVolume.h"
#include "Core/NAAssetSettings.h"

namespace NAHorseTuning
{
	constexpr float Shape = 100.f;
	constexpr float GrazeSpeed = 170.f;
	constexpr float TrotSpeed = 560.f;
	constexpr float GallopSpeed = 1080.f;
	constexpr float FleeSpeed = 950.f;
	constexpr float CallSpeed = 700.f;
	constexpr float SpookRadius = 1400.f;
	constexpr float SpookPlayerSpeed = 300.f; // Walking (180) and crouching are calm; running spooks.
	constexpr float TrustPerFeed = 0.34f;
	constexpr float GallopDrainPerSecond = 15.f;
	constexpr float StaminaRegenPerSecond = 10.f;
	constexpr float WhistleRange = 15000.f;
	const FVector HipPositions[4] = { FVector(70.f, -22.f, 5.f), FVector(70.f, 22.f, 5.f), FVector(-70.f, -22.f, 5.f), FVector(-70.f, 22.f, 5.f) };
	// 0-3 horse coats, 4 deer.
	const FLinearColor Coats[5] = { FLinearColor(0.30f, 0.15f, 0.07f), FLinearColor(0.04f, 0.035f, 0.03f),
		FLinearColor(0.45f, 0.44f, 0.42f), FLinearColor(0.42f, 0.18f, 0.06f), FLinearColor(0.40f, 0.25f, 0.12f) };
	const TCHAR* CoatNames[5] = { TEXT("bay"), TEXT("black"), TEXT("grey"), TEXT("chestnut"), TEXT("deer") };
}

ANAHorse::ANAHorse(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AAIController::StaticClass(); // Lets the server steer it with movement input while wild.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(60.f, 100.f);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 260.f, 0.f);
	Move->MaxWalkSpeed = NAHorseTuning::GrazeSpeed;
	Move->MaxAcceleration = 1400.f;
	Move->BrakingDecelerationWalking = 1200.f;
	Move->JumpZVelocity = 620.f;
	Move->MaxStepHeight = 60.f;
	Move->SetWalkableFloorAngle(48.f);
	Move->NavAgentProps.bCanCrouch = false;

	GetMesh()->SetVisibility(false);

	HealthComponent = CreateDefaultSubobject<UNAHealthComponent>(TEXT("Health"));
	HealthComponent->SetMaxHealth(300.f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(Capsule);
	CameraBoom->TargetArmLength = 520.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 160.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.f;
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* CubeShape = Cube.Object;
	UStaticMesh* SphereShape = Sphere.Object;
	UStaticMesh* CylinderShape = Cylinder.Object;

	// Body: ~2.3 m long, back at ~1.6 m. Feet at the bottom of the capsule (Z -100).
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(Capsule);
	MakePart(TEXT("Barrel"), VisualRoot, SphereShape, FVector(0.f, 0.f, 42.f), FVector(185.f, 62.f, 72.f), FRotator::ZeroRotator, CoatParts);
	MakePart(TEXT("Chest"), VisualRoot, SphereShape, FVector(62.f, 0.f, 46.f), FVector(80.f, 60.f, 78.f), FRotator::ZeroRotator, CoatParts);
	MakePart(TEXT("Rump"), VisualRoot, SphereShape, FVector(-66.f, 0.f, 48.f), FVector(78.f, 62.f, 74.f), FRotator::ZeroRotator, CoatParts);

	NeckJoint = CreateDefaultSubobject<USceneComponent>(TEXT("NeckJoint"));
	NeckJoint->SetupAttachment(VisualRoot);
	NeckJoint->SetRelativeLocation(FVector(88.f, 0.f, 62.f));
	MakePart(TEXT("Neck"), NeckJoint, CylinderShape, FVector(22.f, 0.f, 30.f), FVector(34.f, 28.f, 80.f), FRotator(-38.f, 0.f, 0.f), CoatParts);
	MakePart(TEXT("Head"), NeckJoint, CubeShape, FVector(60.f, 0.f, 70.f), FVector(62.f, 24.f, 26.f), FRotator(-45.f, 0.f, 0.f), CoatParts);
	MakePart(TEXT("Muzzle"), NeckJoint, SphereShape, FVector(80.f, 0.f, 48.f), FVector(28.f, 22.f, 22.f), FRotator::ZeroRotator, ManeParts);
	MakePart(TEXT("EarL"), NeckJoint, CubeShape, FVector(42.f, -8.f, 96.f), FVector(5.f, 5.f, 14.f), FRotator(-10.f, 0.f, -8.f), CoatParts);
	MakePart(TEXT("EarR"), NeckJoint, CubeShape, FVector(42.f, 8.f, 96.f), FVector(5.f, 5.f, 14.f), FRotator(-10.f, 0.f, 8.f), CoatParts);
	MakePart(TEXT("Mane"), NeckJoint, CubeShape, FVector(12.f, 0.f, 46.f), FVector(12.f, 8.f, 76.f), FRotator(-38.f, 0.f, 0.f), ManeParts);
	MakePart(TEXT("Bridle"), NeckJoint, CubeShape, FVector(66.f, 0.f, 58.f), FVector(10.f, 27.f, 6.f), FRotator(-45.f, 0.f, 0.f), TackParts);

	TailJoint = CreateDefaultSubobject<USceneComponent>(TEXT("TailJoint"));
	TailJoint->SetupAttachment(VisualRoot);
	TailJoint->SetRelativeLocation(FVector(-102.f, 0.f, 62.f));
	MakePart(TEXT("Tail"), TailJoint, CylinderShape, FVector(-8.f, 0.f, -32.f), FVector(12.f, 12.f, 70.f), FRotator(-15.f, 0.f, 0.f), ManeParts);

	SaddlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("SaddlePoint"));
	SaddlePoint->SetupAttachment(VisualRoot);
	SaddlePoint->SetRelativeLocation(FVector(-5.f, 0.f, 84.f));
	MakePart(TEXT("Saddle"), VisualRoot, CubeShape, FVector(-5.f, 0.f, 78.f), FVector(52.f, 64.f, 10.f), FRotator::ZeroRotator, TackParts);
	MakePart(TEXT("SaddleBlanket"), VisualRoot, CubeShape, FVector(-5.f, 0.f, 74.f), FVector(62.f, 70.f, 4.f), FRotator::ZeroRotator, TackParts);

	for (int32 Leg = 0; Leg < 4; ++Leg)
	{
		USceneComponent* Hip = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("Hip_%d"), Leg));
		Hip->SetupAttachment(VisualRoot);
		Hip->SetRelativeLocation(NAHorseTuning::HipPositions[Leg]);
		MakePart(*FString::Printf(TEXT("Upper_%d"), Leg), Hip, CylinderShape, FVector(0.f, 0.f, -26.f), FVector(17.f, 17.f, 56.f), FRotator::ZeroRotator, CoatParts);
		USceneComponent* Knee = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("Knee_%d"), Leg));
		Knee->SetupAttachment(Hip);
		Knee->SetRelativeLocation(FVector(0.f, 0.f, -52.f));
		MakePart(*FString::Printf(TEXT("Lower_%d"), Leg), Knee, CylinderShape, FVector(0.f, 0.f, -24.f), FVector(10.f, 10.f, 50.f), FRotator::ZeroRotator, CoatParts);
		MakePart(*FString::Printf(TEXT("Hoof_%d"), Leg), Knee, CylinderShape, FVector(2.f, 0.f, -52.f), FVector(15.f, 15.f, 9.f), FRotator::ZeroRotator, HoofParts);
		Hips.Add(Hip);
		Knees.Add(Knee);
	}
}

UStaticMeshComponent* ANAHorse::MakePart(FName PartName, USceneComponent* Parent, UStaticMesh* Shape, const FVector& Location,
	const FVector& SizeCm, const FRotator& Rotation, TArray<TObjectPtr<UStaticMeshComponent>>& Group)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
	Part->SetupAttachment(Parent);
	Part->SetStaticMesh(Shape);
	Part->SetRelativeLocationAndRotation(Location, Rotation);
	Part->SetRelativeScale3D(SizeCm / NAHorseTuning::Shape);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision); // The capsule handles hits and interaction.
	Part->SetGenerateOverlapEvents(false);
	if (ShapeMaterial.Succeeded()) { Part->SetMaterial(0, ShapeMaterial.Object); }
	Group.Add(Part);
	return Part;
}

void ANAHorse::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANAHorse, bTamed);
	DOREPLIFETIME(ANAHorse, OwnerName);
	DOREPLIFETIME(ANAHorse, Trust);
	DOREPLIFETIME(ANAHorse, Stamina);
	DOREPLIFETIME(ANAHorse, bGalloping);
	DOREPLIFETIME(ANAHorse, CoatIndex);
	DOREPLIFETIME(ANAHorse, Rider);
}

void ANAHorse::BeginPlay()
{
	Super::BeginPlay();
	if (HerdCenter.IsZero())
	{
		HerdCenter = GetActorLocation();
	}
	OnRep_Coat();
	OnRep_Tamed();
	if (NAAssets::ApplyModel(GetModelSlot(), GetMesh(), GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()))
	{
		if (USceneComponent* Shapes = GetShapeRoot()) { Shapes->SetVisibility(false, true); }
	}
	for (UStaticMeshComponent* Part : HoofParts)
	{
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.045f, 0.04f));
		}
	}
	for (UStaticMeshComponent* Part : TackParts)
	{
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.16f, 0.08f, 0.03f)); // Leather.
		}
	}
	if (HasAuthority())
	{
		HealthComponent->OnKilled.AddDynamic(this, &ANAHorse::HandleKilled);
	}
}

void ANAHorse::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() && Rider)
	{
		Dismount(); // Never leave a player stuck in the saddle.
	}
	Super::EndPlay(EndPlayReason);
}

bool ANAHorse::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

float ANAHorse::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Damage <= 0.f || !HealthComponent)
	{
		return 0.f;
	}
	if (!bTamed && EventInstigator && EventInstigator->GetPawn())
	{
		// Wild horses bolt away from whoever hurt them.
		FleeUntil = GetWorld()->GetTimeSeconds() + 6.f;
		FleeDirection = (GetActorLocation() - EventInstigator->GetPawn()->GetActorLocation()).GetSafeNormal2D();
		Trust = FMath::Max(0.f, Trust - 0.5f);
	}
	return HealthComponent->ApplyDamage(Damage, ENAHitZone::Body, EventInstigator, DamageCauser);
}

void ANAHorse::HandleKilled(AActor* Victim, AController* Killer)
{
	Dismount();
	GetCharacterMovement()->DisableMovement();
	VisualRoot->SetRelativeRotation(FRotator(0.f, 0.f, 80.f));
	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, -60.f));
	DropLoot();
	SetLifeSpan(60.f);
	if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
	{
		GameMode->OnHorseKilled(this, Killer);
	}
}

const FNAModelSlot& ANAHorse::GetModelSlot() const
{
	return UNAAssetSettings::Get()->Horse;
}

bool ANAHorse::HasFood(const UNAInventoryComponent* Inventory) const
{
	return Inventory && (Inventory->CountOf(NAItems::Berries()) > 0 || Inventory->CountOf(NAItems::Herb()) > 0);
}

bool ANAHorse::ConsumeFood(UNAInventoryComponent* Inventory) const
{
	return Inventory && (Inventory->RemoveItem(NAItems::Berries(), 1) || Inventory->RemoveItem(NAItems::Herb(), 1));
}

void ANAHorse::DropLoot()
{
	ANAItemPickup::Drop(GetWorld(), GetActorLocation() + GetActorRightVector() * 150.f, NAItems::RawMeat(), 4);
}

bool ANAHorse::ShouldSpookFrom(const ANAPlayerCharacter* Player, float Distance) const
{
	return Distance < NAHorseTuning::SpookRadius && Player->GetVelocity().Size2D() > NAHorseTuning::SpookPlayerSpeed;
}

float ANAHorse::GetFleeSpeed() const
{
	return NAHorseTuning::FleeSpeed;
}

// ----- Interaction: feed (tame) or ride ----------------------------------------------------------

FText ANAHorse::GetInteractionPrompt(const ANACharacterBase* User) const
{
	const ANAPlayerCharacter* Player = Cast<ANAPlayerCharacter>(User);
	const ANAPlayerState* State = Player ? Player->GetPlayerState<ANAPlayerState>() : nullptr;
	if (bTamed)
	{
		const bool bMine = State && State->GetPlayerName() == OwnerName;
		return FText::FromString(bMine ? FString::Printf(TEXT("Ride your %s"), *GetMountName()) : FString::Printf(TEXT("%s's %s"), *OwnerName, *GetMountName()));
	}
	const UNAInventoryComponent* Inventory = Player ? Player->GetInventoryComponent() : nullptr;
	const int32 TrustPercent = FMath::RoundToInt(Trust * 100.f);
	return FText::FromString(HasFood(Inventory)
		? FString::Printf(TEXT("Feed the wild %s (trust %d%%)"), *GetMountName(), TrustPercent)
		: FString::Printf(TEXT("Wild %s (trust %d%%) - %s; approach slowly"), *GetMountName(), TrustPercent, *FoodHint()));
}

bool ANAHorse::CanInteract(const ANACharacterBase* User) const
{
	return CanBeTamed() && !IsDead() && !Rider && Cast<ANAPlayerCharacter>(User) != nullptr;
}

void ANAHorse::Interact(ANACharacterBase* User)
{
	ANAPlayerCharacter* Player = Cast<ANAPlayerCharacter>(User);
	ANAPlayerState* State = Player ? Player->GetPlayerState<ANAPlayerState>() : nullptr;
	if (!HasAuthority() || !CanInteract(User) || !State)
	{
		return;
	}
	ANAPlayerController* PC = Cast<ANAPlayerController>(Player->GetController());
	auto Tell = [PC](const FString& Text)
	{
		if (PC) { PC->ClientReceiveChat(ENAChatChannel::System, FString(), Text); }
	};

	if (bTamed)
	{
		if (State->GetStableId() == OwnerId)
		{
			Mount(Player);
		}
		else
		{
			Tell(FString::Printf(TEXT("This %s trusts only %s."), *GetMountName(), *OwnerName));
		}
		return;
	}

	UNAInventoryComponent* Inventory = Player->GetInventoryComponent();
	if (!Inventory || !ConsumeFood(Inventory))
	{
		Tell(FString::Printf(TEXT("The %s sniffs your empty hand: %s."), *GetMountName(), *FoodHint()));
		return;
	}
	Trust = FMath::Min(1.f, Trust + NAHorseTuning::TrustPerFeed);
	EatUntil = GetWorld()->GetTimeSeconds() + 3.f;
	FleeUntil = 0.f;
	if (Trust >= 0.999f)
	{
		bTamed = true;
		OwnerId = State->GetStableId();
		OwnerName = State->GetPlayerName();
		OnRep_Tamed();
		Tell(FString::Printf(TEXT("You tamed a %s! E to ride, H to whistle for it."), *GetMountName()));
		if (ANAGameMode* GameMode = GetWorld()->GetAuthGameMode<ANAGameMode>())
		{
			GameMode->AwardXP(Player->GetController(), 50, TEXT("Tamed a horse"));
		}
	}
	else
	{
		Tell(FString::Printf(TEXT("The %s eats from your hand. Trust %d%%."), *GetMountName(), FMath::RoundToInt(Trust * 100.f)));
	}
}

// ----- Riding --------------------------------------------------------------------------------------

void ANAHorse::Mount(ANAPlayerCharacter* NewRider)
{
	AController* RiderController = NewRider ? NewRider->GetController() : nullptr;
	if (!HasAuthority() || !RiderController || Rider || NewRider->IsDead() || NewRider->IsDowned() || NewRider->GetCarriedPlayer())
	{
		return;
	}
	if (AController* Brain = GetController())
	{
		Brain->UnPossess();
		Brain->Destroy();
	}
	Rider = NewRider;
	NewRider->SetMountedHorse(this);
	RiderController->Possess(this); // Unpossesses the rider; the player now steers the horse.
	CallTarget = nullptr;
	ApplyRiderAttachment(NewRider, true);
	UpdateSpeed();
}

void ANAHorse::Dismount()
{
	if (!HasAuthority() || !Rider)
	{
		return;
	}
	ANAPlayerCharacter* Leaving = Rider;
	AController* RiderController = GetController();
	Rider = nullptr;
	bGalloping = false;
	Leaving->SetMountedHorse(nullptr);
	ApplyRiderAttachment(Leaving, false);

	// Step down on the left side, on the ground.
	FVector Place = GetActorLocation() - GetActorRightVector() * 130.f;
	FHitResult Ground;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NAHorseDismount), false, this);
	Params.AddIgnoredActor(Leaving);
	if (GetWorld()->LineTraceSingleByChannel(Ground, Place + FVector(0.f, 0.f, 150.f), Place - FVector(0.f, 0.f, 400.f), ECC_Visibility, Params))
	{
		Place = Ground.ImpactPoint + FVector(0.f, 0.f, 95.f);
	}
	Leaving->TeleportTo(Place, FRotator(0.f, GetActorRotation().Yaw, 0.f), false, true);

	if (RiderController)
	{
		RiderController->Possess(Leaving); // Back on foot with the normal controls.
	}
	if (!IsDead() && !IsActorBeingDestroyed())
	{
		SpawnDefaultController(); // Back under the server's care.
	}
	UpdateSpeed();
}

void ANAHorse::OnRep_Rider()
{
	ANAPlayerCharacter* Previous = SeatedLocally.Get();
	if (Previous && Previous != Rider)
	{
		ApplyRiderAttachment(Previous, false);
	}
	if (Rider)
	{
		ApplyRiderAttachment(Rider, true);
	}
	SeatedLocally = Rider;
}

void ANAHorse::ApplyRiderAttachment(ANAPlayerCharacter* Seated, bool bSeat)
{
	if (!Seated)
	{
		return;
	}
	UCharacterMovementComponent* RiderMove = Seated->GetCharacterMovement();
	if (bSeat)
	{
		RiderMove->StopMovementImmediately();
		RiderMove->DisableMovement();
		Seated->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Seated->AttachToComponent(SaddlePoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Seated->SetActorRelativeLocation(FVector(0.f, 0.f, -2.f)); // Hips on the saddle.
		Seated->SetMountedPose(true);
	}
	else
	{
		Seated->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Seated->SetActorRotation(FRotator(0.f, Seated->GetActorRotation().Yaw, 0.f));
		Seated->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		if (!Seated->IsDead())
		{
			RiderMove->SetMovementMode(MOVE_Walking);
		}
		Seated->SetMountedPose(false);
	}
}

void ANAHorse::ServerDismount_Implementation()
{
	Dismount();
}

void ANAHorse::ServerSetGallop_Implementation(bool bNewGallop)
{
	bGalloping = bNewGallop && Rider && Stamina > 5.f;
	UpdateSpeed();
}

void ANAHorse::OnRep_Gallop()
{
	UpdateSpeed();
}

void ANAHorse::OnRep_Tamed()
{
	for (UStaticMeshComponent* Part : TackParts)
	{
		Part->SetVisibility(bTamed);
	}
}

void ANAHorse::OnRep_Coat()
{
	const FLinearColor Coat = NAHorseTuning::Coats[CoatIndex % 5];
	for (UStaticMeshComponent* Part : CoatParts)
	{
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Coat);
		}
	}
	for (UStaticMeshComponent* Part : ManeParts)
	{
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Coat * 0.35f);
		}
	}
}

void ANAHorse::CallTo(APawn* Caller)
{
	if (HasAuthority() && bTamed && !Rider && !IsDead())
	{
		CallTarget = Caller;
		CallUntil = GetWorld()->GetTimeSeconds() + 30.f;
	}
}

ANAHorse* ANAHorse::FindOwnedHorse(UWorld* World, const FString& Id)
{
	if (!World || Id.IsEmpty())
	{
		return nullptr;
	}
	for (TActorIterator<ANAHorse> It(World); It; ++It)
	{
		if (It->bTamed && !It->IsDead() && It->OwnerId == Id)
		{
			return *It;
		}
	}
	return nullptr;
}

// ----- Movement ------------------------------------------------------------------------------------

float ANAHorse::TerrainSpeedFactor() const
{
	float Factor = 1.f;
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Move->CurrentFloor.IsWalkableFloor())
	{
		// Steep ground slows it (0.55 at the walkable limit).
		const float Steepness = 1.f - Move->CurrentFloor.HitResult.ImpactNormal.Z; // 0 flat .. ~0.33
		Factor *= FMath::GetMappedRangeValueClamped(FVector2D(0.04f, 0.33f), FVector2D(1.f, 0.55f), Steepness);
	}
	float SurfaceZ = 0.f;
	if (ANAWaterVolume::FindWaterAt(GetWorld(), GetActorLocation(), SurfaceZ) && SurfaceZ > GetActorLocation().Z - 80.f)
	{
		Factor *= 0.45f; // Wading.
	}
	return Factor;
}

void ANAHorse::UpdateSpeed()
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	float Speed = NAHorseTuning::GrazeSpeed;
	if (Rider)
	{
		Speed = bGalloping ? NAHorseTuning::GallopSpeed : NAHorseTuning::TrotSpeed;
	}
	else if (Now < FleeUntil)
	{
		Speed = GetFleeSpeed();
	}
	else if (CallTarget.IsValid())
	{
		Speed = NAHorseTuning::CallSpeed;
	}
	GetCharacterMovement()->MaxWalkSpeed = Speed * TerrainSpeedFactor();
}

void ANAHorse::MoveToward(const FVector& Destination, float Scale)
{
	const FVector Direction = (Destination - GetActorLocation()).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		AddMovementInput(Direction, Scale);
	}
}

void ANAHorse::ThinkServer(float DeltaSeconds)
{
	const float Now = GetWorld()->GetTimeSeconds();

	// Stamina: galloping drains it, anything else recovers.
	if (Rider && bGalloping && GetVelocity().Size2D() > 50.f)
	{
		Stamina = FMath::Max(0.f, Stamina - NAHorseTuning::GallopDrainPerSecond * DeltaSeconds);
		if (Stamina <= 0.f)
		{
			bGalloping = false; // Winded.
		}
	}
	else
	{
		Stamina = FMath::Min(MaxStamina, Stamina + NAHorseTuning::StaminaRegenPerSecond * DeltaSeconds);
	}

	if (Rider)
	{
		// Throw the rider if they go down (handled on their side too) or die.
		if (Rider->IsDead() || Rider->IsDowned())
		{
			Dismount();
		}
		return;
	}

	if (Now < EatUntil)
	{
		return; // Eating from someone's hand.
	}

	if (bTamed)
	{
		APawn* Caller = CallTarget.Get();
		if (Caller && Now < CallUntil)
		{
			if (FVector::Dist2D(Caller->GetActorLocation(), GetActorLocation()) > 300.f)
			{
				MoveToward(Caller->GetActorLocation(), 1.f);
			}
			else
			{
				CallTarget = nullptr; // Arrived.
			}
		}
		else
		{
			CallTarget = nullptr;
		}
		return;
	}

	// Wild: bolt from anyone running nearby.
	if (Now >= FleeUntil)
	{
		for (TActorIterator<ANAPlayerCharacter> It(GetWorld()); It; ++It)
		{
			if (It->IsDead() || It->IsCloaked())
			{
				continue;
			}
			const float Distance = FVector::Dist2D(It->GetActorLocation(), GetActorLocation());
			if (ShouldSpookFrom(*It, Distance))
			{
				FleeUntil = Now + 4.f;
				FleeDirection = (GetActorLocation() - It->GetActorLocation()).GetSafeNormal2D();
				Trust = FMath::Max(0.f, Trust - 0.15f);
				break;
			}
		}
	}
	if (Now < FleeUntil)
	{
		AddMovementInput(FleeDirection, 1.f);
		return;
	}

	// Graze and wander around the herd.
	if (Now >= NextWanderTime)
	{
		NextWanderTime = Now + FMath::FRandRange(6.f, 12.f);
		const FVector2D Offset = FMath::RandPointInCircle(1500.f);
		WanderTarget = HerdCenter + FVector(Offset.X, Offset.Y, 0.f);
	}
	if (FVector::Dist2D(WanderTarget, GetActorLocation()) > 150.f && Now < NextWanderTime - 2.f)
	{
		MoveToward(WanderTarget, 1.f);
	}
}

void ANAHorse::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsDead())
	{
		return;
	}
	if (HasAuthority())
	{
		ThinkServer(DeltaSeconds);
	}
	if (HasAuthority() || IsLocallyControlled())
	{
		UpdateSpeed(); // Server and the rider's machine agree on speed (terrain + gallop).
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		AnimateBody(DeltaSeconds);
	}
}

void ANAHorse::AnimateBody(float DeltaSeconds)
{
	const float Speed = GetVelocity().Size2D();
	const float Amount = FMath::Clamp(Speed / 500.f, 0.f, 1.4f);
	GaitPhase = FMath::Fmod(GaitPhase + DeltaSeconds * (2.f + Speed / 90.f), 2.f * PI);
	const bool bGallopGait = Speed > 800.f;
	for (int32 Leg = 0; Leg < Hips.Num(); ++Leg)
	{
		// Trot: diagonal pairs together. Gallop: front pair, then back pair.
		const bool bFront = Leg < 2;
		const float Offset = bGallopGait ? (bFront ? 0.f : PI * 0.6f) + (Leg % 2) * 0.35f : ((Leg == 0 || Leg == 3) ? 0.f : PI);
		const float Swing = FMath::Sin(GaitPhase + Offset);
		Hips[Leg]->SetRelativeRotation(FRotator(Swing * 30.f * Amount, 0.f, 0.f));
		Knees[Leg]->SetRelativeRotation(FRotator((bFront ? 1.f : -1.f) * FMath::Max(0.f, -Swing) * 55.f * Amount, 0.f, 0.f));
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bGrazing = !Rider && Speed < 30.f && FMath::Sin(Now * 0.25f + CoatIndex) > 0.2f;
	const float NeckPitch = bGrazing ? -55.f : FMath::Sin(GaitPhase * 2.f) * 5.f * Amount;
	NeckJoint->SetRelativeRotation(FMath::RInterpTo(NeckJoint->GetRelativeRotation(), FRotator(NeckPitch, 0.f, 0.f), DeltaSeconds, 3.f));
	TailJoint->SetRelativeRotation(FRotator(-Amount * 25.f, FMath::Sin(Now * 1.7f) * 12.f, 0.f));
	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, FMath::Abs(FMath::Sin(GaitPhase)) * 6.f * Amount));
}

// ----- Input (rider's machine) --------------------------------------------------------------------

void ANAHorse::BuildInput()
{
	if (RideContext)
	{
		return;
	}
	auto MakeAction = [this](const TCHAR* ActionName, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, ActionName, RF_Transient);
		Action->ValueType = Type;
		return Action;
	};
	MoveAction = MakeAction(TEXT("IA_HorseMove"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_HorseLook"), EInputActionValueType::Axis2D);
	GallopAction = MakeAction(TEXT("IA_HorseGallop"), EInputActionValueType::Boolean);
	JumpAction = MakeAction(TEXT("IA_HorseJump"), EInputActionValueType::Boolean);
	DismountAction = MakeAction(TEXT("IA_HorseDismount"), EInputActionValueType::Boolean);
	DescendAction = MakeAction(TEXT("IA_MountDescend"), EInputActionValueType::Boolean);

	RideContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Horse"), RF_Transient);
	UInputMappingContext* IMC = RideContext;
	auto Negate = [IMC](bool bX, bool bY)
	{
		UInputModifierNegate* Mod = NewObject<UInputModifierNegate>(IMC);
		Mod->bX = bX;
		Mod->bY = bY;
		Mod->bZ = false;
		return Mod;
	};
	auto Swizzle = [IMC]() { return NewObject<UInputModifierSwizzleAxis>(IMC); }; // Default order YXZ: key drives Y.

	IMC->MapKey(MoveAction, EKeys::W).Modifiers.Add(Swizzle());
	{
		FEnhancedActionKeyMapping& Back = IMC->MapKey(MoveAction, EKeys::S);
		Back.Modifiers.Add(Swizzle());
		Back.Modifiers.Add(Negate(true, true));
	}
	IMC->MapKey(MoveAction, EKeys::A).Modifiers.Add(Negate(true, true));
	IMC->MapKey(MoveAction, EKeys::D);
	IMC->MapKey(MoveAction, EKeys::Gamepad_Left2D).Modifiers.Add(NewObject<UInputModifierDeadZone>(IMC));
	IMC->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(Negate(false, true));
	IMC->MapKey(GallopAction, EKeys::LeftShift);
	IMC->MapKey(GallopAction, EKeys::Gamepad_LeftThumbstick);
	IMC->MapKey(JumpAction, EKeys::SpaceBar);
	IMC->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
	IMC->MapKey(DescendAction, EKeys::C);
	IMC->MapKey(DescendAction, EKeys::LeftControl);
	IMC->MapKey(DismountAction, EKeys::E);
	IMC->MapKey(DismountAction, EKeys::Gamepad_FaceButton_Top);
}

void ANAHorse::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInput();
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANAHorse::Input_Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANAHorse::Input_Look);
	Input->BindAction(GallopAction, ETriggerEvent::Started, this, &ANAHorse::Input_GallopStarted);
	Input->BindAction(GallopAction, ETriggerEvent::Completed, this, &ANAHorse::Input_GallopCompleted);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ANAHorse::Input_Jump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ANAHorse::Input_JumpReleased);
	Input->BindAction(DescendAction, ETriggerEvent::Started, this, &ANAHorse::Input_DescendStarted);
	Input->BindAction(DescendAction, ETriggerEvent::Completed, this, &ANAHorse::Input_DescendCompleted);
	Input->BindAction(DismountAction, ETriggerEvent::Started, this, &ANAHorse::Input_Dismount);
}

void ANAHorse::PawnClientRestart()
{
	Super::PawnClientRestart();
	BuildInput();
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			// The player's on-foot controls come back when they dismount (their body clears this context).
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(RideContext, 0);
		}
	}
}

void ANAHorse::Input_Move(const FInputActionValue& Value)
{
	if (!GetController())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotationMatrix YawMatrix(FRotator(0.f, GetController()->GetControlRotation().Yaw, 0.f));
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void ANAHorse::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ANAHorse::Input_GallopStarted()
{
	if (Stamina > 5.f)
	{
		bGalloping = true; // Predict locally; the server confirms.
		UpdateSpeed();
	}
	ServerSetGallop(true);
}

void ANAHorse::Input_GallopCompleted()
{
	bGalloping = false;
	UpdateSpeed();
	ServerSetGallop(false);
}

void ANAHorse::Input_Jump()
{
	Jump();
}

void ANAHorse::Input_Dismount()
{
	ServerDismount();
}
