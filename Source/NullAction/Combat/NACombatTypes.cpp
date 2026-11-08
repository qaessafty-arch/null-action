#include "Combat/NACombatTypes.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"

ENAHitZone NACombat::GetHitZone(const FHitResult& Hit)
{
	if (Hit.BoneName != NAME_None)
	{
		const FString Bone = Hit.BoneName.ToString().ToLower();
		if (Bone.Contains(TEXT("head")) || Bone.Contains(TEXT("neck")))
		{
			return ENAHitZone::Head;
		}

		static const TCHAR* LimbTokens[] = { TEXT("arm"), TEXT("hand"), TEXT("thigh"), TEXT("calf"), TEXT("foot"), TEXT("leg"), TEXT("clavicle"), TEXT("ball") };
		for (const TCHAR* Token : LimbTokens)
		{
			if (Bone.Contains(Token))
			{
				return ENAHitZone::Limb;
			}
		}
		return ENAHitZone::Body;
	}

	// Creatures and other hand-built bodies tag their hittable parts.
	if (const UPrimitiveComponent* Part = Hit.GetComponent())
	{
		if (Part->ComponentHasTag(TEXT("NA.Head")))
		{
			return ENAHitZone::Head;
		}
		if (Part->ComponentHasTag(TEXT("NA.Limb")))
		{
			return ENAHitZone::Limb;
		}
	}

	// Stand-in bodies are hit on the capsule: top of the capsule is the head, bottom third the legs.
	if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Hit.GetComponent()))
	{
		const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		if (HalfHeight > KINDA_SMALL_NUMBER)
		{
			const float RelativeHeight = (Hit.ImpactPoint.Z - Capsule->GetComponentLocation().Z) / HalfHeight;
			if (RelativeHeight > 0.62f)
			{
				return ENAHitZone::Head;
			}
			if (RelativeHeight < -0.3f)
			{
				return ENAHitZone::Limb;
			}
		}
	}
	return ENAHitZone::Body;
}
