// Implementation: Create a MO56SaveGame asset via the save subsystem; inventories and
// world actors register automatically. When adding new systems, extend this data container
// with serializable fields and update UMO56SaveSubsystem to read/write them.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InventoryComponent.h"
#include "ItemPickup.h"
#include "Skills/SkillTypes.h"
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
 * Save data for a single connected player.
 */
USTRUCT(BlueprintType)
struct FPlayerSaveData
{
        GENERATED_BODY()

        /** Persistent identifier generated for the player controller. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
        FGuid PlayerId;

        /** Persistent identifier of the player's inventory component. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
        FGuid InventoryId;

        /** Numeric controller id recorded at save time for matching reconnects. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
        int32 ControllerId = INDEX_NONE;

        /** Display name captured from the player state. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
        FString PlayerName;

        /** Last known world transform for the possessed pawn. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
        FTransform Transform = FTransform::Identity;

        /** Serialized skill system state. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
        FSkillSystemSaveData SkillState;
};

/**
 * Save game asset for the MO56 project.
 */
UCLASS()
class MO56_API UMO56SaveGame : public USaveGame
{
        GENERATED_BODY()

public:
        /** Timestamp recording when the save file was first created. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FDateTime InitialSaveTimestamp;

        /** Timestamp of the most recent time the save file was written. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FDateTime LastSaveTimestamp;

        /** Accumulated play time stored with the save (in seconds). */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        float TotalPlayTimeSeconds = 0.f;

        /** The last level name recorded when this save was written. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FName LastLevelName;

        /** Serialized inventory states keyed by persistent identifier. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        TMap<FGuid, FInventorySaveData> InventoryStates;

        /** Level-specific world state used for persistence. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        TMap<FName, FLevelWorldState> LevelStates;

        /** Inventories belonging to player pawns at the time of save. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        TSet<FGuid> PlayerInventoryIds;

        /** World transforms recorded for each player inventory. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        TMap<FGuid, FTransform> PlayerTransforms;

        /** Per-player save state including controller information and skills. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        TMap<FGuid, FPlayerSaveData> PlayerStates;
};

