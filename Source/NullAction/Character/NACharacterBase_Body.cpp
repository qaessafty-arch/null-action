// Stand-in humanoid body for characters without a skeletal mesh: a jointed figure built from engine
// shapes, animated procedurally (walk/run cycle, crouch, aim pose, lying down when prone, downed or dead).
#include "Character/NACharacterBase.h"

#include "Combat/NAHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace NABody
{
	constexpr float Shape = 100.f;     // Engine basic shapes are 100 cm across.
	constexpr float PelvisHeight = 96.f;
	constexpr float CrouchPelvisHeight = 62.f;
	enum class EPaint : uint8 { Skin, Outfit, Pants, Gear, Accent };
}

void ANACharacterBase::BuildFallbackHumanoid()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UStaticMesh* Cube = CubeMesh.Object;
	UStaticMesh* Cylinder = CylinderMesh.Object;
	UStaticMesh* Sphere = SphereMesh.Object;
	UMaterialInterface* Material = ShapeMaterial.Object;

	auto Joint = [this](const TCHAR* JointName, USceneComponent* Parent, const FVector& Location)
	{
		USceneComponent* Node = CreateDefaultSubobject<USceneComponent>(JointName);
		Node->SetupAttachment(Parent);
		Node->SetRelativeLocation(Location);
		return Node;
	};
	auto Part = [this, Material](const TCHAR* PartName, USceneComponent* Parent, UStaticMesh* ShapeMesh, const FVector& Location,
		const FVector& SizeCm, NABody::EPaint Paint, UStaticMeshComponent* Existing = nullptr)
	{
		UStaticMeshComponent* Piece = Existing ? Existing : CreateDefaultSubobject<UStaticMeshComponent>(PartName);
		Piece->SetupAttachment(Parent);
		if (ShapeMesh) { Piece->SetStaticMesh(ShapeMesh); }
		if (Material) { Piece->SetMaterial(0, Material); }
		Piece->SetRelativeLocation(Location);
		Piece->SetRelativeScale3D(SizeCm / NABody::Shape);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetGenerateOverlapEvents(false);
		switch (Paint)
		{
		case NABody::EPaint::Skin:   FallbackSkinParts.Add(Piece); break;
		case NABody::EPaint::Outfit: FallbackOutfitParts.Add(Piece); break;
		case NABody::EPaint::Pants:  FallbackPantsParts.Add(Piece); break;
		case NABody::EPaint::Gear:   FallbackGearParts.Add(Piece); break;
		default: break; // Accent: the visor, coloured separately.
		}
		return Piece;
	};

	// Root sits at the feet (bottom of the capsule); X forward, Y right, Z up.
	FallbackRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FallbackRoot"));
	FallbackRoot->SetupAttachment(GetCapsuleComponent());

	FallbackPelvis = Joint(TEXT("FB_Pelvis"), FallbackRoot, FVector(0.f, 0.f, NABody::PelvisHeight));
	Part(TEXT("FB_Hips"), FallbackPelvis, Cube, FVector(0.f, 0.f, 0.f), FVector(24.f, 36.f, 18.f), NABody::EPaint::Pants);
	Part(TEXT("FB_Belt"), FallbackPelvis, Cube, FVector(0.f, 0.f, 9.f), FVector(26.f, 38.f, 5.f), NABody::EPaint::Gear);

	FallbackSpine = Joint(TEXT("FB_Spine"), FallbackPelvis, FVector(0.f, 0.f, 10.f));
	FallbackBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallbackBody"));
	Part(nullptr, FallbackSpine, Cube, FVector(0.f, 0.f, 26.f), FVector(24.f, 40.f, 50.f), NABody::EPaint::Outfit, FallbackBody);
	Part(TEXT("FB_Vest"), FallbackSpine, Cube, FVector(3.f, 0.f, 28.f), FVector(26.f, 34.f, 34.f), NABody::EPaint::Gear);
	Part(TEXT("FB_Pack"), FallbackSpine, Cube, FVector(-17.f, 0.f, 30.f), FVector(12.f, 28.f, 32.f), NABody::EPaint::Outfit);

	FallbackNeck = Joint(TEXT("FB_Neck"), FallbackSpine, FVector(0.f, 0.f, 52.f));
	Part(TEXT("FB_NeckPart"), FallbackNeck, Cylinder, FVector(0.f, 0.f, 4.f), FVector(11.f, 11.f, 10.f), NABody::EPaint::Skin);
	Part(TEXT("FB_Head"), FallbackNeck, Sphere, FVector(1.f, 0.f, 19.f), FVector(21.f, 19.f, 24.f), NABody::EPaint::Skin);
	Part(TEXT("FB_Helmet"), FallbackNeck, Sphere, FVector(-1.f, 0.f, 25.f), FVector(23.f, 22.f, 16.f), NABody::EPaint::Outfit);
	FallbackVisor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallbackVisor"));
	Part(nullptr, FallbackNeck, Cube, FVector(9.f, 0.f, 21.f), FVector(5.f, 16.f, 5.f), NABody::EPaint::Accent, FallbackVisor);

	// Arms hang along -Z from the shoulders. Positive pitch swings a limb forward.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Y = Side == 0 ? -1.f : 1.f;
		const TCHAR* S = Side == 0 ? TEXT("L") : TEXT("R");
		USceneComponent* Shoulder = Joint(*FString::Printf(TEXT("FB_Shoulder%s"), S), FallbackSpine, FVector(0.f, 25.f * Y, 46.f));
		Part(*FString::Printf(TEXT("FB_Pad%s"), S), Shoulder, Sphere, FVector(0.f, 0.f, 0.f), FVector(13.f, 13.f, 11.f), NABody::EPaint::Gear);
		Part(*FString::Printf(TEXT("FB_UpperArm%s"), S), Shoulder, Cylinder, FVector(0.f, 0.f, -14.f), FVector(10.f, 10.f, 28.f), NABody::EPaint::Outfit);
		USceneComponent* Elbow = Joint(*FString::Printf(TEXT("FB_Elbow%s"), S), Shoulder, FVector(0.f, 0.f, -28.f));
		Part(*FString::Printf(TEXT("FB_Forearm%s"), S), Elbow, Cylinder, FVector(0.f, 0.f, -12.f), FVector(9.f, 9.f, 25.f), NABody::EPaint::Outfit);
		Part(*FString::Printf(TEXT("FB_Hand%s"), S), Elbow, Sphere, FVector(0.f, 0.f, -28.f), FVector(9.f, 8.f, 10.f), NABody::EPaint::Gear);
		FallbackShoulders.Add(Shoulder);
		FallbackElbows.Add(Elbow);

		USceneComponent* Hip = Joint(*FString::Printf(TEXT("FB_Hip%s"), S), FallbackPelvis, FVector(0.f, 10.f * Y, -4.f));
		Part(*FString::Printf(TEXT("FB_Thigh%s"), S), Hip, Cylinder, FVector(0.f, 0.f, -22.f), FVector(15.f, 15.f, 44.f), NABody::EPaint::Pants);
		USceneComponent* Knee = Joint(*FString::Printf(TEXT("FB_Knee%s"), S), Hip, FVector(0.f, 0.f, -44.f));
		Part(*FString::Printf(TEXT("FB_KneePad%s"), S), Knee, Sphere, FVector(5.f, 0.f, 0.f), FVector(9.f, 13.f, 11.f), NABody::EPaint::Gear);
		Part(*FString::Printf(TEXT("FB_Shin%s"), S), Knee, Cylinder, FVector(0.f, 0.f, -20.f), FVector(12.f, 12.f, 40.f), NABody::EPaint::Pants);
		Part(*FString::Printf(TEXT("FB_Boot%s"), S), Knee, Cube, FVector(5.f, 0.f, -44.f), FVector(25.f, 13.f, 10.f), NABody::EPaint::Gear);
		FallbackHips.Add(Hip);
		FallbackKnees.Add(Knee);
	}
}

void ANACharacterBase::ApplyFallbackColors()
{
	auto Paint = [](const TArray<TObjectPtr<UStaticMeshComponent>>& Parts, const FLinearColor& Color)
	{
		for (UStaticMeshComponent* Piece : Parts)
		{
			if (UMaterialInstanceDynamic* MID = Piece ? Piece->CreateAndSetMaterialInstanceDynamic(0) : nullptr)
			{
				MID->SetVectorParameterValue(TEXT("Color"), Color);
			}
		}
	};
	Paint(FallbackSkinParts, FallbackSkinColor);
	Paint(FallbackOutfitParts, FallbackOutfitColor);
	Paint(FallbackPantsParts, FallbackPantsColor);
	Paint(FallbackGearParts, FallbackGearColor);
	if (UMaterialInstanceDynamic* MID = FallbackVisor->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FallbackAccentColor);
	}
}

void ANACharacterBase::SetFallbackVisible(bool bVisible)
{
	FallbackRoot->SetVisibility(bVisible, /*bPropagateToChildren*/ true);
}

void ANACharacterBase::UpdateFallbackBodyHeight()
{
	// Whole-body placement for the current state; limbs are animated every frame in AnimateFallbackBody.
	const float HalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	if (IsDead())
	{
		// On its back, body stretching behind the feet.
		FallbackRoot->SetRelativeLocationAndRotation(FVector(80.f, 0.f, -HalfHeight + 14.f), FRotator(86.f, 0.f, 0.f));
	}
	else if (bIsProne || IsDowned())
	{
		// Face down, body stretching forward from the feet, centred on the capsule.
		FallbackRoot->SetRelativeLocationAndRotation(FVector(-85.f, 0.f, -HalfHeight + 14.f), FRotator(-86.f, 0.f, 0.f));
	}
	else
	{
		FallbackRoot->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -HalfHeight), FRotator::ZeroRotator);
	}
}

void ANACharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() != NM_DedicatedServer && FallbackRoot && FallbackRoot->IsVisible() && !HasRealBody())
	{
		AnimateFallbackBody(DeltaSeconds);
	}
}

void ANACharacterBase::AnimateFallbackBody(float DeltaSeconds)
{
	if (FallbackShoulders.Num() < 2 || FallbackHips.Num() < 2)
	{
		return;
	}
	if (bMountedPose)
	{
		// Riding: astride the saddle, legs down the horse's sides, hands forward on the reins.
		FallbackPelvis->SetRelativeLocation(FVector(0.f, 0.f, NABody::PelvisHeight));
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float Out = Side == 0 ? -1.f : 1.f;
			FallbackHips[Side]->SetRelativeRotation(FRotator(30.f, 0.f, 38.f * Out));
			FallbackKnees[Side]->SetRelativeRotation(FRotator(-55.f, 0.f, -20.f * Out));
			FallbackShoulders[Side]->SetRelativeRotation(FRotator(45.f, 0.f, 4.f * Out));
			FallbackElbows[Side]->SetRelativeRotation(FRotator(45.f, 0.f, 0.f));
		}
		FallbackSpine->SetRelativeRotation(FRotator(-6.f, 0.f, 0.f));
		FallbackNeck->SetRelativeRotation(FRotator::ZeroRotator);
		return;
	}
	const bool bDeadNow = IsDead();
	const bool bLying = bDeadNow || bIsProne || IsDowned();
	const bool bCrouching = bIsCrouched && !bLying;
	const float Speed = bDeadNow ? 0.f : GetVelocity().Size2D();
	const float Amount = FMath::Clamp(Speed / 380.f, 0.f, 1.3f);

	// One full stride (left + right step) every ~1.5 m.
	FallbackWalkPhase = FMath::Fmod(FallbackWalkPhase + DeltaSeconds * Speed * (2.f * PI / 150.f), 2.f * PI);
	const float Swing = FMath::Sin(FallbackWalkPhase);
	const float Bob = FMath::Abs(FMath::Cos(FallbackWalkPhase)) * 3.f * Amount;

	const float PelvisZ = bCrouching ? NABody::CrouchPelvisHeight : NABody::PelvisHeight;
	FallbackPelvis->SetRelativeLocation(FVector(0.f, 0.f, PelvisZ + (bLying ? 0.f : Bob)));

	// Legs: alternate swing, knee bends on the back-swing; crouch folds them.
	const float HipBase = bCrouching ? 55.f : 0.f;
	const float KneeBase = bCrouching ? -100.f : 0.f;
	const float LegSwing = (bLying ? 12.f : 32.f) * Amount;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Phase = Side == 0 ? Swing : -Swing;
		const float KneeBend = FMath::Max(0.f, -Phase) * 50.f * Amount;
		FallbackHips[Side]->SetRelativeRotation(FRotator(bDeadNow ? 4.f : HipBase + Phase * LegSwing, 0.f, Side == 0 ? -2.f : 2.f));
		FallbackKnees[Side]->SetRelativeRotation(FRotator(bDeadNow ? -6.f : KneeBase - KneeBend - (bCrouching ? 0.f : 6.f * Amount), 0.f, 0.f));
	}

	// Torso: lean into a sprint, lean forward when crouched.
	const float Lean = bLying ? 0.f : (IsSprinting() ? -12.f : bCrouching ? -14.f : -3.f * Amount);
	FallbackSpine->SetRelativeRotation(FRotator(Lean, 0.f, 0.f));
	FallbackNeck->SetRelativeRotation(FRotator(bLying && !bDeadNow ? 70.f : -Lean * 0.6f, 0.f, 0.f)); // Look ahead when lying face down.

	// Arms: hold the weapon up (higher when aiming), otherwise swing opposite the legs.
	const bool bArmed = CurrentWeapon != nullptr && !bDeadNow && !IsDowned();
	for (int32 Side = 0; Side < 2; ++Side)
	{
		float ShoulderPitch;
		float ShoulderRoll = Side == 0 ? -6.f : 6.f;
		float ElbowPitch;
		if (bDeadNow)
		{
			ShoulderPitch = 0.f;
			ShoulderRoll = Side == 0 ? -35.f : 35.f;
			ElbowPitch = 10.f;
		}
		else if (bArmed)
		{
			const bool bRight = Side == 1;
			ShoulderPitch = bIsAiming ? (bRight ? 88.f : 80.f) : (bRight ? 55.f : 45.f);
			ElbowPitch = bIsAiming ? (bRight ? 5.f : 30.f) : (bRight ? 35.f : 55.f);
			ShoulderRoll = bRight ? 4.f : 28.f; // Support hand reaches across to the gun.
		}
		else
		{
			const float Phase = Side == 0 ? -Swing : Swing;
			ShoulderPitch = bLying ? 150.f + Phase * 20.f * Amount : Phase * 28.f * Amount;
			ElbowPitch = 12.f + FMath::Max(0.f, Phase) * 30.f * Amount;
		}
		FallbackShoulders[Side]->SetRelativeRotation(FRotator(ShoulderPitch, 0.f, ShoulderRoll));
		FallbackElbows[Side]->SetRelativeRotation(FRotator(ElbowPitch, 0.f, 0.f));
	}
}
