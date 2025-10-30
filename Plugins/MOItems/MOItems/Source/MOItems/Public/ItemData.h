#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPtr.h"
#include "ItemData.generated.h"

class UTexture2D;
class UStaticMesh;
class USkeletalMesh;
class AActor;

UCLASS(BlueprintType)
class MOITEMS_API UItemData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item", meta=(ClampMin="1"))
	int32 MaxStackSize = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSoftObjectPtr<UTexture2D> Icon;

	// World visuals for pickups
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
	TSoftObjectPtr<UStaticMesh>   WorldStaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
	TSoftObjectPtr<USkeletalMesh> WorldSkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
	FVector  WorldScale3D = FVector(1.f);

        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
        FRotator WorldRotationOffset = FRotator::ZeroRotator;

        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World", meta=(AllowedClasses="/Script/MOInventory.ItemPickup"))
        TSoftClassPtr<AActor> PickupActorClass;

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Item"), GetFName());
	}
};
