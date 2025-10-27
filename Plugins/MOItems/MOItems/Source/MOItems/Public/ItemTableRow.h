#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemTableRow.generated.h"

class UItemData;
class UTexture2D;
class UStaticMesh;
class USkeletalMesh;
class AActor;

USTRUCT(BlueprintType)
struct MOITEMS_API FItemTableRow : public FTableRowBase
{
    GENERATED_BODY()

    // Canonical gameplay data asset (your existing UItemData)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
    TSoftObjectPtr<UItemData> ItemAsset;

    // Optional override display name for table row listings or UI
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
    FText DisplayNameOverride;

    // Visuals for world pickup or UI
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
    TSoftObjectPtr<UStaticMesh> StaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    // Optional default pickup actor if you want custom visuals/behavior per item
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup")
    TSoftClassPtr<AActor> PickupActorClass;

    // Quick overrides if you want to drive stack size from table
    // If <= 0, use ItemAsset->MaxStackSize
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gameplay", meta=(ClampMin="0"))
    int32 MaxStackOverride = 0;
};
