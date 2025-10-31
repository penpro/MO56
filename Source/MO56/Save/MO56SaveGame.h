#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InventoryComponent.h"
#include "ItemPickup.h"
#include "MO56SaveGame.generated.h"

/**
 * Serializable information describing a single world pickup.
 */
USTRUCT(BlueprintType)
struct FWorldItemSaveData
{
        GENERATED_BODY()

        /** Unique identifier for the pickup. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        FGuid PickupId;

        /** Soft reference to the item definition. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        FSoftObjectPath ItemPath;

        /** Class used when spawning the pickup into the world. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        TSoftClassPtr<AItemPickup> PickupClass;

        /** World transform recorded at save time. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        FTransform Transform = FTransform::Identity;

        /** Quantity held by the pickup. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        int32 Quantity = 0;

        /** True when the pickup originated from an inventory drop. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        bool bSpawnedFromInventory = false;
};

/**
 * Aggregated world state for a single level.
 */
USTRUCT(BlueprintType)
struct FLevelWorldState
{
        GENERATED_BODY()

        /** Pickups that should exist when the level is loaded. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        TArray<FWorldItemSaveData> DroppedItems;

        /** Pickups that were removed from the level and should not respawn. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        TSet<FGuid> RemovedPickupIds;

        /** Reserved set for future PCG removals (trees, rocks, etc.). */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        TSet<FGuid> RemovedProceduralIds;
};

/**
 * Save game asset for the MO56 project.
 */
UCLASS()
class MO56_API UMO56SaveGame : public USaveGame
{
        GENERATED_BODY()

public:
        /** Serialized inventory states keyed by persistent identifier. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        TMap<FGuid, FInventorySaveData> InventoryStates;

        /** Level-specific world state used for persistence. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        TMap<FName, FLevelWorldState> LevelStates;

        /** Identifier for the primary player inventory. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        FGuid PlayerInventoryId;

        /** Last known transform of the player character. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        FTransform PlayerTransform = FTransform::Identity;
};

