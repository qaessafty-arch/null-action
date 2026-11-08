#include "Creatures/NASkyMount.h"

#include "Character/NAPlayerCharacter.h"
#include "Combat/NAHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NAPlayerController.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Inventory/NAInventoryComponent.h"
#include "Inventory/NAItems.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "World/NAWorldEnvironment.h"
#include "Core/NAAssetSettings.h"

namespace NASky
{
	constexpr float WalkSpeed = 300.f;
	constexpr float FlySpeed = 1500.f;
	constexpr float BoostSpeed = 2600.f;
	constexpr float CircleRadius = 2500.f;
	constexpr float CircleHeight = 2500.f;
	constexpr float Ceiling = 14000.f;
	constexpr float LandHeight = 160.f;
	constexpr float FlapDrainPerSecond = 9.f;
}

ANASkyMount::ANASkyMount(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HealthComponent->SetMaxHealth(400.f);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxFlySpeed = NASky::FlySpeed;
	Move->BrakingDecelerationFlying = 250.f; // Keeps momentum: gliding.
	Move->MaxAcceleration = 1800.f;
	Move->RotationRate = FRotator(0.f, 140.f, 0.f);
	CameraBoom->TargetArmLength = 850.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 220.f);

	// Hide the horse body; this creature has its own.
	VisualRoot->SetVisibility(false, true);
	CoatParts.Empty();
	ManeParts.Empty();
	HoofParts.Empty();
	TackParts.Empty();
	SaddlePoint->SetRelativeLocation(FVector(10.f, 0.f, 70.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	GliderRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GliderRoot"));
	GliderRoot->SetupAttachment(GetCapsuleComponent());
	GliderRoot->SetRelativeLocation(FVector(0.f, 0.f, -20.f));
	// A long ray-like body with a crested head, two vast wings and a whip tail. Wingspan ~9 m.
	MakePart(TEXT("G_Body"), GliderRoot, Sphere.Object, FVector(0.f, 0.f, 20.f), FVector(260.f, 120.f, 75.f), FRotator::ZeroRotator, GliderAsh);
	MakePart(TEXT("G_Head"), GliderRoot, Sphere.Object, FVector(155.f, 0.f, 40.f), FVector(80.f, 55.f, 50.f), FRotator::ZeroRotator, GliderAsh);
	MakePart(TEXT("G_Beak"), GliderRoot, Cone.Object, FVector(205.f, 0.f, 36.f), FVector(30.f, 30.f, 60.f), FRotator(-90.f, 0.f, 0.f), GliderAsh);
	MakePart(TEXT("G_Crest"), GliderRoot, Cone.Object, FVector(130.f, 0.f, 80.f), FVector(20.f, 50.f, 70.f), FRotator(-60.f, 0.f, 0.f), GliderEmber);
	MakePart(TEXT("G_EyeL"), GliderRoot, Sphere.Object, FVector(180.f, -22.f, 50.f), FVector(10.f), FRotator::ZeroRotator, GliderEmber);
	MakePart(TEXT("G_EyeR"), GliderRoot, Sphere.Object, FVector(180.f, 22.f, 50.f), FVector(10.f), FRotator::ZeroRotator, GliderEmber);
	MakePart(TEXT("G_Tail"), GliderRoot, Cone.Object, FVector(-210.f, 0.f, 20.f), FVector(28.f, 28.f, 200.f), FRotator(90.f, 0.f, 0.f), GliderAsh);
	MakePart(TEXT("G_TailFin"), GliderRoot, Cube.Object, FVector(-300.f, 0.f, 20.f), FVector(50.f, 90.f, 6.f), FRotator::ZeroRotator, GliderEmber);
	MakePart(TEXT("G_LegL"), GliderRoot, Cylinder.Object, FVector(20.f, -35.f, -30.f), FVector(14.f, 14.f, 70.f), FRotator::ZeroRotator, GliderAsh);
	MakePart(TEXT("G_LegR"), GliderRoot, Cylinder.Object, FVector(20.f, 35.f, -30.f), FVector(14.f, 14.f, 70.f), FRotator::ZeroRotator, GliderAsh);
	MakePart(TEXT("G_Saddle"), GliderRoot, Cube.Object, FVector(10.f, 0.f, 62.f), FVector(60.f, 60.f, 10.f), FRotator::ZeroRotator, GliderTack);

	WingLeft = CreateDefaultSubobject<USceneComponent>(TEXT("WingLeft"));
	WingLeft->SetupAttachment(GliderRoot);
	WingLeft->SetRelativeLocation(FVector(20.f, -50.f, 35.f));
	WingRight = CreateDefaultSubobject<USceneComponent>(TEXT("WingRight"));
	WingRight->SetupAttachment(GliderRoot);
	WingRight->SetRelativeLocation(FVector(20.f, 50.f, 35.f));
	MakePart(TEXT("G_WingL"), WingLeft, Cube.Object, FVector(-10.f, -210.f, 0.f), FVector(150.f, 420.f, 8.f), FRotator(0.f, -8.f, 0.f), GliderAsh);
	MakePart(TEXT("G_WingTipL"), WingLeft, Cube.Object, FVector(-40.f, -430.f, 0.f), FVector(80.f, 60.f, 7.f), FRotator(0.f, -20.f, 0.f), GliderEmber);
	MakePart(TEXT("G_WingR"), WingRight, Cube.Object, FVector(-10.f, 210.f, 0.f), FVector(150.f, 420.f, 8.f), FRotator(0.f, 8.f, 0.f), GliderAsh);
	MakePart(TEXT("G_WingTipR"), WingRight, Cube.Object, FVector(-40.f, 430.f, 0.f), FVector(80.f, 60.f, 7.f), FRotator(0.f, 20.f, 0.f), GliderEmber);
}

void ANASkyMount::BeginPlay()
{
	Super::BeginPlay();
	auto Paint = [](const TArray<TObjectPtr<UStaticMeshComponent>>& Parts, const FLinearColor& Color)
	{
		for (UStaticMeshComponent* Part : Parts)
		{
			if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
			{
				MID->SetVectorParameterValue(TEXT("Color"), Color);
			}
		}
	};
	Paint(GliderAsh, FLinearColor(0.16f, 0.15f, 0.15f));
	Paint(GliderEmber, FLinearColor(1.f, 0.38f, 0.06f));
	Paint(GliderTack, FLinearColor(0.16f, 0.08f, 0.03f));
	PhaseEndTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(15.f, 30.f);
}

bool ANASkyMount::IsFlying() const
{
	return GetCharacterMovement()->MovementMode == MOVE_Flying;
}

float ANASkyMount::GetAltitudeAboveGround() const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NASkyAltitude), false, this);
	if (GetRider()) { Params.AddIgnoredActor(GetRider()); }
	const FVector Start = GetActorLocation();
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.f, 0.f, 50000.f), ECC_Visibility, Params))
	{
		return FMath::Max(0.f, Start.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - Hit.ImpactPoint.Z);
	}
	return 50000.f;
}

bool ANASkyMount::IsInStorm() const
{
	const ANAWorldEnvironment* Environment = ANAWorldEnvironment::Get(this);
	if (!Environment)
	{
		return false;
	}
	const ENAWeather Weather = Environment->GetWeather();
	return Weather == ENAWeather::Thunderstorm || Weather == ENAWeather::Blizzard || Weather == ENAWeather::DustStorm || Weather == ENAWeather::Windy;
}

bool ANASkyMount::HasFood(const UNAInventoryComponent* Inventory) const
{
	return Inventory && (Inventory->CountOf(NAItems::RawMeat()) > 0 || Inventory->CountOf(TEXT("CookedMeat")) > 0);
}

bool ANASkyMount::ConsumeFood(UNAInventoryComponent* Inventory) const
{
	return Inventory && (Inventory->RemoveItem(NAItems::RawMeat(), 1) || Inventory->RemoveItem(TEXT("CookedMeat"), 1));
}

bool ANASkyMount::CanInteract(const ANACharacterBase* User) const
{
	// Only reachable (and tameable) while it's on the ground.
	return Super::CanInteract(User) && !IsFlying();
}

// ----- Flight --------------------------------------------------------------------------------------

void ANASkyMount::TakeOff()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Move->MovementMode != MOVE_Flying)
	{
		Move->SetMovementMode(MOVE_Flying);
		Move->Velocity += FVector(0.f, 0.f, 700.f) + GetActorForwardVector() * 300.f;
	}
}

void ANASkyMount::Land()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Move->MovementMode == MOVE_Flying)
	{
		Move->SetMovementMode(MOVE_Falling); // Drops the last few metres onto its feet.
	}
}

void ANASkyMount::ServerTakeOff_Implementation()
{
	if (Stamina > 10.f)
	{
		TakeOff();
	}
}

void ANASkyMount::ServerLand_Implementation()
{
	if (GetAltitudeAboveGround() < 450.f)
	{
		Land();
	}
}

void ANASkyMount::FlyToward(const FVector& Destination, float Scale)
{
	const FVector Direction = (Destination - GetActorLocation()).GetSafeNormal();
	AddMovementInput(Direction, Scale);
}

void ANASkyMount::UpdateSpeed()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const float Weather = IsInStorm() ? 0.75f : 1.f;
	Move->MaxFlySpeed = (GetRider() && bGalloping ? NASky::BoostSpeed : NASky::FlySpeed) * Weather;
	Move->MaxWalkSpeed = NASky::WalkSpeed;
}

void ANASkyMount::ThinkServer(float DeltaSeconds)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bFlying = IsFlying();
	const bool bFlapping = bFlying && (GetVelocity().Z > 120.f || (bGalloping && GetRider()));

	// Stamina: flapping (climbing / boosting) costs it; gliding and resting recover it.
	if (bFlapping)
	{
		Stamina = FMath::Max(0.f, Stamina - NASky::FlapDrainPerSecond * (bGalloping ? 1.8f : 1.f) * DeltaSeconds);
		if (Stamina <= 0.f) { bGalloping = false; }
	}
	else
	{
		Stamina = FMath::Min(MaxStamina, Stamina + (bFlying ? 4.f : 12.f) * DeltaSeconds);
	}

	if (GetRider())
	{
		if (GetRider()->IsDead() || GetRider()->IsDowned())
		{
			Dismount(); // Thrown off (placed on the ground below).
		}
		return;
	}
	if (Now < EatUntil)
	{
		return;
	}

	// Called by the owner's whistle: fly over and land beside them.
	if (bTamed)
	{
		APawn* Caller = CallTarget.Get();
		if (Caller && Now < CallUntil)
		{
			const float Flat = FVector::Dist2D(Caller->GetActorLocation(), GetActorLocation());
			if (Flat > 500.f)
			{
				if (!bFlying) { TakeOff(); }
				FlyToward(Caller->GetActorLocation() + FVector(0.f, 0.f, 600.f), 1.f);
			}
			else if (bFlying)
			{
				AddMovementInput(FVector::DownVector, 1.f);
				if (GetAltitudeAboveGround() < NASky::LandHeight) { Land(); CallTarget = nullptr; }
			}
			else
			{
				CallTarget = nullptr;
			}
		}
		else if (bFlying)
		{
			AddMovementInput(FVector::DownVector, 0.6f); // Left in the air: glide down and wait.
			if (GetAltitudeAboveGround() < NASky::LandHeight) { Land(); }
		}
		return;
	}

	// Wild: spooked from its perch -> take off.
	if (!bFlying)
	{
		for (TActorIterator<ANAPlayerCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->IsDead() && ShouldSpookFrom(*It, FVector::Dist2D(It->GetActorLocation(), GetActorLocation())))
			{
				PhaseEndTime = Now; // Leave now.
				Trust = FMath::Max(0.f, Trust - 0.15f);
				break;
			}
		}
	}

	if (Now >= PhaseEndTime)
	{
		bWildAirborne = !bWildAirborne;
		PhaseEndTime = Now + (bWildAirborne ? FMath::FRandRange(35.f, 55.f) : FMath::FRandRange(25.f, 40.f));
		if (bWildAirborne) { TakeOff(); }
	}
	if (bWildAirborne)
	{
		// Circle high above the roost.
		if (!bFlying) { TakeOff(); }
		CircleAngle = FMath::Fmod(CircleAngle + DeltaSeconds * 0.25f, 2.f * PI);
		const FVector Point = HerdCenter + FVector(FMath::Cos(CircleAngle), FMath::Sin(CircleAngle), 0.f) * NASky::CircleRadius + FVector(0.f, 0.f, NASky::CircleHeight);
		FlyToward(Point, 1.f);
	}
	else if (bFlying)
	{
		// Come home and land on the roost.
		const FVector Roost = HerdCenter + FVector(0.f, 0.f, 200.f);
		if (FVector::Dist2D(Roost, GetActorLocation()) > 300.f)
		{
			FlyToward(Roost + FVector(0.f, 0.f, 400.f), 1.f);
		}
		else
		{
			AddMovementInput(FVector::DownVector, 1.f);
			if (GetAltitudeAboveGround() < NASky::LandHeight) { Land(); }
		}
	}
}

void ANASkyMount::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds); // Horse tick: ThinkServer (server), UpdateSpeed, AnimateBody.
	if (IsDead() || !IsLocallyControlled() || !IsFlying())
	{
		return;
	}
	// Rider's machine: climbing, diving, gliding, storms and landing.
	const float Altitude = GetAltitudeAboveGround();
	if (bClimbHeld && Stamina > 0.f && GetActorLocation().Z < NASky::Ceiling)
	{
		AddMovementInput(FVector::UpVector, 1.f);
	}
	else if (bDiveHeld)
	{
		AddMovementInput(FVector::DownVector, 1.f);
	}
	else
	{
		AddMovementInput(FVector::DownVector, Stamina > 0.f ? 0.15f : 0.5f); // Gliding sinks; exhausted sinks faster.
	}
	if (IsInStorm())
	{
		// Gusts push you around.
		const float Time = GetWorld()->GetTimeSeconds();
		AddMovementInput(FVector(FMath::Sin(Time * 1.3f), FMath::Cos(Time * 0.9f), FMath::Sin(Time * 2.1f) * 0.5f), 0.35f);
	}
	if (Altitude < NASky::LandHeight && !bClimbHeld && GetVelocity().Z <= 0.f)
	{
		Land();
		ServerLand();
	}
}

void ANASkyMount::AnimateBody(float DeltaSeconds)
{
	const bool bFlying = IsFlying();
	const float ClimbRate = FMath::Clamp(GetVelocity().Z / 600.f, -1.f, 1.f);
	// Flap hard when climbing or taking off; hold the wings out when gliding; fold them on the ground.
	const float FlapSpeed = bFlying ? (ClimbRate > 0.1f || bGalloping ? 7.f : 1.2f) : 0.6f;
	WingPhase = FMath::Fmod(WingPhase + DeltaSeconds * FlapSpeed, 2.f * PI);
	const float Flap = bFlying ? FMath::Sin(WingPhase) * (ClimbRate > 0.1f || bGalloping ? 35.f : 6.f) : 0.f;
	const float Fold = bFlying ? 0.f : 55.f;
	WingLeft->SetRelativeRotation(FRotator(0.f, -Fold, Flap));
	WingRight->SetRelativeRotation(FRotator(0.f, Fold, -Flap));
	const float Bank = bFlying ? FMath::Clamp(GetCharacterMovement()->GetLastInputVector().Y * 20.f, -25.f, 25.f) : 0.f;
	GliderRoot->SetRelativeRotation(FMath::RInterpTo(GliderRoot->GetRelativeRotation(), FRotator(-ClimbRate * 15.f, 0.f, Bank), DeltaSeconds, 3.f));
	for (UStaticMeshComponent* Part : GliderTack)
	{
		Part->SetVisibility(bTamed);
	}
}

// ----- Rider input ---------------------------------------------------------------------------------

void ANASkyMount::Input_Move(const FInputActionValue& Value)
{
	if (!IsFlying())
	{
		Super::Input_Move(Value); // Walks like the horse on the ground.
		return;
	}
	if (!GetController())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotationMatrix Look(GetController()->GetControlRotation());
	AddMovementInput(Look.GetUnitAxis(EAxis::X), Axis.Y); // Fly where you look.
	AddMovementInput(Look.GetUnitAxis(EAxis::Y), Axis.X);
}

void ANASkyMount::Input_Jump()
{
	if (!IsFlying())
	{
		if (Stamina > 10.f)
		{
			TakeOff(); // Predict locally; the server confirms.
			ServerTakeOff();
		}
		return;
	}
	bClimbHeld = true;
}

void ANASkyMount::Input_JumpReleased()
{
	bClimbHeld = false;
}

void ANASkyMount::Input_DescendStarted()
{
	bDiveHeld = true;
}

void ANASkyMount::Input_DescendCompleted()
{
	bDiveHeld = false;
}

void ANASkyMount::Input_Dismount()
{
	if (IsFlying())
	{
		if (GetAltitudeAboveGround() < 450.f)
		{
			Land();
			ServerLand();
		}
		else if (ANAPlayerController* PC = Cast<ANAPlayerController>(GetController()))
		{
			PC->ClientReceiveChat(ENAChatChannel::System, FString(), TEXT("Too high to dismount. Dive (C) and land first."));
		}
		return;
	}
	Super::Input_Dismount();
}

const FNAModelSlot& ANASkyMount::GetModelSlot() const
{
	return UNAAssetSettings::Get()->AshGlider;
}
