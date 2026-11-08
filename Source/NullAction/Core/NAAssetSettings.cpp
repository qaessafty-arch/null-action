#include "Core/NAAssetSettings.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"

namespace
{
	bool PackageExists(const FSoftObjectPath& Path)
	{
		return Path.IsValid() && FPackageName::DoesPackageExist(Path.GetLongPackageName());
	}
}

bool NAAssets::ApplyModel(const FNAModelSlot& Slot, USkeletalMeshComponent* MeshComponent, float CapsuleHalfHeight)
{
	if (!MeshComponent || !Slot.IsSet() || !PackageExists(Slot.Mesh.ToSoftObjectPath()))
	{
		return false;
	}
	USkeletalMesh* Loaded = Slot.Mesh.LoadSynchronous();
	if (!Loaded)
	{
		return false;
	}
	MeshComponent->SetSkeletalMeshAsset(Loaded);
	MeshComponent->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -CapsuleHalfHeight) + Slot.Offset, Slot.Rotation);
	MeshComponent->SetRelativeScale3D(FVector(FMath::Max(0.01f, Slot.Scale)));
	MeshComponent->SetVisibility(true);
	if (!Slot.AnimClass.IsNull() && PackageExists(Slot.AnimClass.ToSoftObjectPath()))
	{
		if (UClass* AnimClass = Slot.AnimClass.LoadSynchronous())
		{
			MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			MeshComponent->SetAnimInstanceClass(AnimClass);
		}
	}
	return true;
}

UStaticMesh* NAAssets::LoadStatic(const TSoftObjectPtr<UStaticMesh>& Path)
{
	if (Path.IsNull() || !PackageExists(Path.ToSoftObjectPath()))
	{
		return nullptr;
	}
	return Path.LoadSynchronous();
}
