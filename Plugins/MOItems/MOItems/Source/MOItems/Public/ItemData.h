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

        /** Optional manual override for the item's weight in kilograms. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Physics", meta=(ClampMin="0", ForceUnits="kg"))
        float WeightKgOverride = 0.f;

        /** Optional manual override for the item's weight in pounds. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Physics", meta=(ClampMin="0", ForceUnits="lb"))
        float WeightLbsOverride = 0.f;

        /** Optional manual override for the item's volume (in cubic meters). */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Physics", meta=(ClampMin="0", ForceUnits="m^3"))
        float VolumeOverride = 0.f;

        /** Optional manual override for the item's density (in kg / m^3). */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Physics", meta=(ClampMin="0", ForceUnits="kg/m^3"))
        float DensityOverride = 0.f;

        /** When true the item will be dropped as a container when spawned into the world. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Behavior")
        bool bDropAsContainer = false;

public:
        virtual FPrimaryAssetId GetPrimaryAssetId() const override
        {
                return FPrimaryAssetId(TEXT("Item"), GetFName());
        }

        /** Returns the weight of the item in kilograms. */
        UFUNCTION(BlueprintCallable, Category="Item|Physics")
        float GetWeightKg() const;

        /** Returns the weight of the item in pounds. */
        UFUNCTION(BlueprintCallable, Category="Item|Physics")
        float GetWeightLbs() const;

        /** Returns the calculated item volume in cubic meters. */
        UFUNCTION(BlueprintCallable, Category="Item|Physics")
        float GetVolumeCubicMeters() const;

        /** Returns the density of the item (kg / m^3). */
        UFUNCTION(BlueprintCallable, Category="Item|Physics")
        float GetDensity() const;

private:
        float CalculateDefaultWeightKg() const;
        float CalculateDefaultVolumeCubicMeters() const;
};
