#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InventoryComponent.h"
#include "ItemPickup.h"
#include "Skills/SkillTypes.h"
#include "Save/MO56SaveTypes.h"
#include "MO56SaveGame.generated.h"

USTRUCT(BlueprintType)
struct FWorldItemSaveData
{
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        FGuid PickupId;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        FSoftObjectPath ItemPath;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        TSoftClassPtr<AItemPickup> PickupClass;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        FTransform Transform = FTransform::Identity;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        int32 Quantity = 0;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        bool bSpawnedFromInventory = false;
};

USTRUCT(BlueprintType)
struct FLevelWorldState
{
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        TArray<FWorldItemSaveData> DroppedItems;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        TSet<FGuid> RemovedPickupIds;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
        TSet<FGuid> RemovedProceduralIds;
};

USTRUCT(BlueprintType)
struct FCharacterSaveData
{
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
        FGuid CharacterId;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
        FGuid InventoryId;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
        FTransform Transform = FTransform::Identity;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
        FSkillSystemSaveData SkillState;
};

USTRUCT(BlueprintType)
struct FPlayerSaveData
{
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
        FGuid PlayerId;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
        int32 ControllerId = INDEX_NONE;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
        FString PlayerName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
        FTransform Transform = FTransform::Identity;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
        FSkillSystemSaveData SkillState;
};

UCLASS()
class MO56_API UMO56SaveGame : public USaveGame
{
        GENERATED_BODY()

public:
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        int32 SaveVersion = MO56_SAVE_VERSION;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FGuid SaveId;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FString SlotName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FString LevelName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FDateTime CreatedUtc;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FDateTime UpdatedUtc;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        float TotalPlayTimeSeconds = 0.f;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        bool bIsGameplaySave = true;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Player")
        FMO56PlayerStats PlayerStats;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|World")
        TMap<FName, FLevelWorldState> LevelStates;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Inventory")
        TMap<FGuid, FInventorySaveData> InventoryStates;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Character")
        TMap<FGuid, FCharacterSaveData> CharacterStates;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Player")
        TMap<FGuid, FPlayerSaveData> PlayerStates;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Legacy")
        TMap<FGuid, FGuid> PlayerInventoryIds;
};

UCLASS()
class MO56_API UMO56SaveMetadata : public USaveGame
{
        GENERATED_BODY()

public:
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        int32 SaveVersion = MO56_SAVE_VERSION;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FGuid SaveId;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FString SlotName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FString LevelName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FDateTime CreatedUtc;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        FDateTime UpdatedUtc;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
        float TotalPlaySeconds = 0.f;
};

UCLASS()
class MO56_API UMO56SaveIndex : public USaveGame
{
        GENERATED_BODY()

public:
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Index")
        TArray<FSaveIndexEntry> Entries;
};

