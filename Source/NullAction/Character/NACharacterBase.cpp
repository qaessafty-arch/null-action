#include "Character/NACharacterBase.h"

#include "NullAction.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/PackageName.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace NACharacter
{
	// Engine basic shapes are 100 units across; the cylinder is centred on its pivot.
	constexpr float ShapeSize = 100.f;
	constexpr float CapsuleRadius = 35.f;
	constexpr float CapsuleHalfHeight = 90.f;
}

ANACharacterBase::ANACharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->InitCapsuleSize(NACharacter::CapsuleRadius, NACharacter::CapsuleHalfHeight);

	// Third-person action setup: the body turns toward its movement, not toward the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->MaxWalkSpeed = RunSpeed;
	Move->MaxWalkSpeedCrouched = CrouchSpeed;
	Move->MaxAcceleration = 1600.f;
	Move->BrakingDecelerationWalking = 1800.f;
	Move->JumpZVelocity = 460.f;
	Move->AirControl = 0.3f;
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->bCanWalkOffLedgesWhenCrouching = true;
	Move->SetCrouchedHalfHeight(55.f);

	// Mannequin-style meshes have their origin at the feet and face +Y.
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -NACharacter::CapsuleHalfHeight), FRotator(0.f, -90.f, 0.f));
	// Keep bones updated on the server too, so future server-side hit validation sees real poses.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	BodyMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny")));
	BodyAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	FallbackBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallbackBody"));
	FallbackBody->SetupAttachment(GetCapsuleComponent());
	FallbackBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FallbackBody->SetGenerateOverlapEvents(false);
	if (CylinderMesh.Succeeded())
	{
		FallbackBody->SetStaticMesh(CylinderMesh.Object);
	}

	// A "visor" block on the face so you can see which way the stand-in body is facing.
	FallbackVisor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallbackVisor"));
	FallbackVisor->SetupAttachment(GetCapsuleComponent());
	FallbackVisor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FallbackVisor->SetGenerateOverlapEvents(false);
	FallbackVisor->SetRelativeScale3D(FVector(0.12f, 0.4f, 0.12f));
	if (CubeMesh.Succeeded())
	{
		FallbackVisor->SetStaticMesh(CubeMesh.Object);
	}

	UpdateFallbackBodyHeight();
}

void ANACharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANACharacterBase, Gait);
}

void ANACharacterBase::BeginPlay()
{
	Super::BeginPlay();
	ApplyGait();
	ResolveAppearance();
}

void ANACharacterBase::SetDesiredGait(ENAGait NewGait)
{
	if (NewGait == ENAGait::Sprint && bIsCrouched)
	{
		UnCrouch();
	}

	if (Gait != NewGait)
	{
		Gait = NewGait;
		ApplyGait();
	}

	if (!HasAuthority())
	{
		ServerSetGait(NewGait);
	}
}

void ANACharacterBase::ServerSetGait_Implementation(ENAGait NewGait)
{
	if (static_cast<uint8>(NewGait) > static_cast<uint8>(ENAGait::Sprint))
	{
		return; // Reject garbage from a modified client.
	}
	SetDesiredGait(NewGait);
}

void ANACharacterBase::OnRep_Gait()
{
	ApplyGait();
}

void ANACharacterBase::ApplyGait()
{
	float Speed = RunSpeed;
	switch (Gait)
	{
	case ENAGait::Walk:   Speed = WalkSpeed;   break;
	case ENAGait::Run:    Speed = RunSpeed;    break;
	case ENAGait::Sprint: Speed = SprintSpeed; break;
	}
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void ANACharacterBase::ToggleCrouch()
{
	if (bIsCrouched)
	{
		UnCrouch();
		return;
	}

	if (Gait == ENAGait::Sprint)
	{
		SetDesiredGait(ENAGait::Run);
	}
	Crouch(); // ACharacter crouch is client-predicted and server-replicated by the movement component.
}

void ANACharacterBase::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	UpdateFallbackBodyHeight();
}

void ANACharacterBase::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	UpdateFallbackBodyHeight();
}

void ANACharacterBase::UpdateFallbackBodyHeight()
{
	const float HalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float Radius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();

	FallbackBody->SetRelativeScale3D(FVector(
		(Radius * 2.f - 10.f) / NACharacter::ShapeSize,
		(Radius * 2.f - 10.f) / NACharacter::ShapeSize,
		(HalfHeight * 2.f) / NACharacter::ShapeSize));

	FallbackVisor->SetRelativeLocation(FVector(Radius - 8.f, 0.f, HalfHeight - 25.f));
}

void ANACharacterBase::ResolveAppearance()
{
	USkeletalMeshComponent* MeshComp = GetMesh();

	// Only touch disk if the asset is actually in the project; a missing pack is expected, not an error.
	auto AssetExists = [](const FSoftObjectPath& Path)
	{
		return Path.IsValid() && FPackageName::DoesPackageExist(Path.GetLongPackageName());
	};

	if (!MeshComp->GetSkeletalMeshAsset() && AssetExists(BodyMesh.ToSoftObjectPath()))
	{
		if (USkeletalMesh* Loaded = BodyMesh.LoadSynchronous())
		{
			MeshComp->SetSkeletalMeshAsset(Loaded);
		}
	}

	if (MeshComp->GetSkeletalMeshAsset() && !MeshComp->GetAnimInstance() && AssetExists(BodyAnimClass.ToSoftObjectPath()))
	{
		if (UClass* AnimClass = BodyAnimClass.LoadSynchronous())
		{
			MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			MeshComp->SetAnimInstanceClass(AnimClass);
		}
	}

	const bool bHasRealBody = MeshComp->GetSkeletalMeshAsset() != nullptr;
	FallbackBody->SetVisibility(!bHasRealBody);
	FallbackVisor->SetVisibility(!bHasRealBody);

	if (!bHasRealBody)
	{
		UE_LOG(LogNullAction, Warning,
			TEXT("%s: no skeletal mesh found at %s. Showing stand-in body. Add the Third Person content pack or set BodyMesh in a Blueprint subclass."),
			*GetName(), *BodyMesh.ToString());
	}
}
