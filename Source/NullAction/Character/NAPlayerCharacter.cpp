#include "Character/NAPlayerCharacter.h"

#include "NullAction.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

ANAPlayerCharacter::ANAPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = StandingArmLength;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f); // Over the right shoulder.
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 14.f;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(85.f);
}

void ANAPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		const bool bMovingFast = IsSprinting() && GetVelocity().Size2D() > RunSpeed;
		const float TargetLength = bMovingFast ? SprintArmLength : StandingArmLength;
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetLength, DeltaSeconds, ArmLengthInterpSpeed);
	}
}

void ANAPlayerCharacter::BuildDefaultInputIfNeeded()
{
	if (MappingContext)
	{
		return; // Designer-authored input in use.
	}

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name, RF_Transient);
		Action->ValueType = Type;
		return Action;
	};

	MoveAction   = MakeAction(TEXT("IA_Move"),   EInputActionValueType::Axis2D);
	LookAction   = MakeAction(TEXT("IA_Look"),   EInputActionValueType::Axis2D);
	JumpAction   = MakeAction(TEXT("IA_Jump"),   EInputActionValueType::Boolean);
	SprintAction = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	WalkAction   = MakeAction(TEXT("IA_Walk"),   EInputActionValueType::Boolean);
	CrouchAction = MakeAction(TEXT("IA_Crouch"), EInputActionValueType::Boolean);

	UInputMappingContext* IMC = NewObject<UInputMappingContext>(this, TEXT("IMC_NullActionDefault"), RF_Transient);

	auto Swizzle = [IMC]() { return NewObject<UInputModifierSwizzleAxis>(IMC); }; // Default order YXZ: key value goes to Y.
	auto Negate = [IMC](bool bX, bool bY)
	{
		UInputModifierNegate* Mod = NewObject<UInputModifierNegate>(IMC);
		Mod->bX = bX;
		Mod->bY = bY;
		Mod->bZ = false;
		return Mod;
	};

	// Move: Y = forward/back, X = right/left.
	IMC->MapKey(MoveAction, EKeys::W).Modifiers.Add(Swizzle());
	{
		FEnhancedActionKeyMapping& S = IMC->MapKey(MoveAction, EKeys::S);
		S.Modifiers.Add(Swizzle());
		S.Modifiers.Add(Negate(true, true));
	}
	IMC->MapKey(MoveAction, EKeys::A).Modifiers.Add(Negate(true, true));
	IMC->MapKey(MoveAction, EKeys::D);
	IMC->MapKey(MoveAction, EKeys::Gamepad_Left2D).Modifiers.Add(NewObject<UInputModifierDeadZone>(IMC));

	// Look: mouse delta, inverted Y so pushing the mouse forward looks up.
	IMC->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(Negate(false, true));
	{
		FEnhancedActionKeyMapping& Stick = IMC->MapKey(LookAction, EKeys::Gamepad_Right2D);
		Stick.Modifiers.Add(NewObject<UInputModifierDeadZone>(IMC));
		Stick.Modifiers.Add(Negate(false, true));
		UInputModifierScalar* Rate = NewObject<UInputModifierScalar>(IMC);
		Rate->Scalar = FVector(60.f, 45.f, 1.f); // Stick deflection -> degrees/sec (before engine look scale).
		Stick.Modifiers.Add(Rate);
		Stick.Modifiers.Add(NewObject<UInputModifierScaleByDeltaTime>(IMC));
	}

	IMC->MapKey(JumpAction, EKeys::SpaceBar);
	IMC->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);

	IMC->MapKey(SprintAction, EKeys::LeftShift);
	IMC->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);

	IMC->MapKey(WalkAction, EKeys::LeftAlt);
	IMC->MapKey(WalkAction, EKeys::Gamepad_DPad_Down);

	IMC->MapKey(CrouchAction, EKeys::C);
	IMC->MapKey(CrouchAction, EKeys::LeftControl);
	IMC->MapKey(CrouchAction, EKeys::Gamepad_FaceButton_Right);

	MappingContext = IMC;
}

void ANAPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildDefaultInputIfNeeded();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogNullAction, Error, TEXT("Enhanced Input is not the default input component. Check Config/DefaultInput.ini."));
		return;
	}

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Input_JumpStarted);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::Input_JumpCompleted);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ThisClass::Input_SprintStarted);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::Input_SprintCompleted);
	Input->BindAction(WalkAction, ETriggerEvent::Started, this, &ThisClass::Input_WalkToggle);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::Input_Crouch);
}

void ANAPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	BuildDefaultInputIfNeeded();

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

void ANAPlayerCharacter::Input_Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FRotationMatrix YawMatrix(YawRotation);

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void ANAPlayerCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ANAPlayerCharacter::Input_JumpStarted()
{
	if (bIsCrouched)
	{
		UnCrouch(); // Jump from crouch stands up first, like most shooters.
		return;
	}
	Jump();
}

void ANAPlayerCharacter::Input_JumpCompleted()
{
	StopJumping();
}

void ANAPlayerCharacter::Input_SprintStarted()
{
	bSprintHeld = true;
	UpdateDesiredGait();
}

void ANAPlayerCharacter::Input_SprintCompleted()
{
	bSprintHeld = false;
	UpdateDesiredGait();
}

void ANAPlayerCharacter::Input_WalkToggle()
{
	bWalkToggled = !bWalkToggled;
	UpdateDesiredGait();
}

void ANAPlayerCharacter::Input_Crouch()
{
	ToggleCrouch();
}

void ANAPlayerCharacter::UpdateDesiredGait()
{
	if (bSprintHeld)
	{
		SetDesiredGait(ENAGait::Sprint);
	}
	else
	{
		SetDesiredGait(bWalkToggled ? ENAGait::Walk : ENAGait::Run);
	}
}
