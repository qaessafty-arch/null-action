#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NAInteractable.h"
#include "NABuildPiece.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class ANADoor;

UENUM(BlueprintType)
enum class ENABuildPieceType : uint8
{
	Foundation,
	Wall,
	Doorway,
	Campfire
};

/**
 * A player-built structure piece. Placed by the server after it checks the builder has the item and
 * the spot is valid. Remembers its owner; only the owner can demolish it (refunds half). Has health,
 * so it can be shot down (raiding). Campfires warm nearby players and count as a cooking station.
 */
UCLASS()
class NULLACTION_API ANABuildPiece : public AActor, public INAInteractable
{
	GENERATED_BODY()

public:
	ANABuildPiece();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual FText GetInteractionPrompt(const ANACharacterBase* User) const override;
	virtual bool CanInteract(const ANACharacterBase* User) const override;
	virtual void Interact(ANACharacterBase* User) override;

	/** Set before FinishSpawning (server). */
	UPROPERTY(Replicated) ENABuildPieceType PieceType = ENABuildPieceType::Foundation;
	UPROPERTY(Replicated) FString OwnerName;
	FString OwnerId; // Server only.

	static FName ItemFor(ENABuildPieceType Type);
	static bool TypeForItem(FName ItemId, ENABuildPieceType& OutType);
	static FVector SizeOf(ENABuildPieceType Type);
	static ANABuildPiece* FindCampfireNear(const UWorld* World, const FVector& Location, float Radius);

	bool IsCampfire() const { return PieceType == ENABuildPieceType::Campfire; }

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	UPROPERTY(Replicated) float Health = 400.f;

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY() TObjectPtr<UPointLightComponent> FireLight;
	void AddFireEffects();
	UPROPERTY() TObjectPtr<ANADoor> SpawnedDoor;

private:
	UStaticMeshComponent* AddPart(const FVector& BottomCentre, const FVector& Size, const FLinearColor& Color, bool bCollision = true);
	void BuildShape();
	float FlickerTime = 0.f;
};
