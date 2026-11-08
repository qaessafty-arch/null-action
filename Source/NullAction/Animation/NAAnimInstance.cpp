#include "Animation/NAAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"

void UNAAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningCharacter = Cast<ANACharacterBase>(TryGetPawnOwner());
}

void UNAAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningCharacter)
	{
		OwningCharacter = Cast<ANACharacterBase>(TryGetPawnOwner());
		if (!OwningCharacter)
		{
			return;
		}
	}

	const UCharacterMovementComponent* Move = OwningCharacter->GetCharacterMovement();
	Velocity = OwningCharacter->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, OwningCharacter->GetActorRotation());
	AimPitch = (OwningCharacter->GetBaseAimRotation() - OwningCharacter->GetActorRotation()).GetNormalized().Pitch;

	bIsFalling = Move && Move->IsFalling();
	bIsCrouching = OwningCharacter->bIsCrouched;
	bIsSprinting = OwningCharacter->IsSprinting();
	Gait = OwningCharacter->GetGait();

	const bool bHasAcceleration = Move && !Move->GetCurrentAcceleration().IsNearlyZero();
	bShouldMove = GroundSpeed > 3.f && bHasAcceleration;
}
