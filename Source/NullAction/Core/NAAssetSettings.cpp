#include "Core/NAAssetSettings.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

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

bool NAAssets::ApplyProp(const FNAPropSlot& Slot, UStaticMeshComponent* Component)
{
	UStaticMesh* Mesh = Component ? LoadStatic(Slot.Mesh) : nullptr;
	if (!Mesh)
	{
		return false;
	}
	Component->SetStaticMesh(Mesh);
	Component->EmptyOverrideMaterials(); // Use the asset's own (realistic) materials.
	Component->SetRelativeLocationAndRotation(Slot.Offset, Slot.Rotation);
	Component->SetRelativeScale3D(FVector(FMath::Max(0.01f, Slot.Scale)));
	return true;
}

USoundBase* NAAssets::LoadSound(const TSoftObjectPtr<USoundBase>& Path)
{
	if (Path.IsNull() || !PackageExists(Path.ToSoftObjectPath()))
	{
		return nullptr;
	}
	return Path.LoadSynchronous();
}

void NAAssets::PlayAt(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Path, const FVector& Location, float Volume)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (USoundBase* Sound = LoadSound(Path))
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContext, Sound, Location, Volume);
	}
}

UNiagaraComponent* NAAssets::SpawnEffect(const UObject* WorldContext, const TSoftObjectPtr<UNiagaraSystem>& Path, const FVector& Location, const FRotator& Rotation)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer || Path.IsNull() || !PackageExists(Path.ToSoftObjectPath()))
	{
		return nullptr;
	}
	UNiagaraSystem* System = Path.LoadSynchronous();
	return System ? UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, Location, Rotation) : nullptr;
}

UNiagaraComponent* NAAssets::SpawnEffectAttached(const TSoftObjectPtr<UNiagaraSystem>& Path, USceneComponent* AttachTo, const FVector& RelativeLocation)
{
	const UWorld* World = AttachTo ? AttachTo->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer || Path.IsNull() || !PackageExists(Path.ToSoftObjectPath()))
	{
		return nullptr;
	}
	UNiagaraSystem* System = Path.LoadSynchronous();
	return System ? UNiagaraFunctionLibrary::SpawnSystemAttached(System, AttachTo, NAME_None, RelativeLocation, FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset, /*bAutoDestroy*/ false) : nullptr;
}
