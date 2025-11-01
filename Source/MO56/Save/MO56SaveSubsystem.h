// Implementation: Game instance subsystem that serializes inventories and pickups. Register
// pawn/container inventory components in BeginPlay so their state is captured; call
// SaveGame/LoadGame from UI or gameplay logic. Extend this subsystem when persisting new
// systems such as quests or crafting.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/MO56SaveGame.h"
#include "MO56SaveSubsystem.generated.h"

class AItemPickup;
class UInventoryComponent;

USTRUCT(BlueprintType)
struct FSaveGameSummary
{
        GENERATED_BODY()

        /** Slot identifier on disk. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        FString SlotName;

        /** User index associated with the save slot. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        int32 UserIndex = 0;

        /** Initial creation timestamp for the save. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        FDateTime InitialSaveTimestamp;

        /** Timestamp of the last save operation. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        FDateTime LastSaveTimestamp;

        /** Total tracked play time in seconds. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        float TotalPlayTimeSeconds = 0.f;

        /** Level recorded when the save was last written. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        FName LastLevelName = NAME_None;

        /** Count of serialized inventories in the save. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        int32 InventoryCount = 0;
};

/**
 * Centralized save-game subsystem that persists inventory and world pickup data.
 */
UCLASS()
class MO56_API UMO56SaveSubsystem : public UGameInstanceSubsystem
{
        GENERATED_BODY()

public:
        UMO56SaveSubsystem();

        virtual void Initialize(FSubsystemCollectionBase& Collection) override;
        virtual void Deinitialize() override;

        /** Saves the current world and inventory state to disk. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        bool SaveGame();

        /** Loads the world and inventory state from disk. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        bool LoadGame();

        /** Loads save data from the specified slot name. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        bool LoadGameBySlot(const FString& SlotName, int32 UserIndex);

        /** Clears the current save data and resets runtime state. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        void ResetToNewGame();

        /** Registers an inventory component so its state is serialized. */
        void RegisterInventoryComponent(UInventoryComponent* InventoryComponent, bool bIsPlayerInventory);

        /** Stops tracking the specified inventory component. */
        void UnregisterInventoryComponent(UInventoryComponent* InventoryComponent);

        /** Registers a pickup actor for persistence tracking. */
        void RegisterWorldPickup(AItemPickup* Pickup);

        /** Unregisters a pickup actor from persistence tracking. */
        void UnregisterWorldPickup(AItemPickup* Pickup);

        /** Returns the active save game object. */
        UFUNCTION(BlueprintPure, Category = "Save")
        UMO56SaveGame* GetCurrentSaveGame() const { return CurrentSaveGame; }

        /** Returns summaries for all discoverable save games. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        TArray<FSaveGameSummary> GetAvailableSaveSummaries() const;

private:
        static constexpr const TCHAR* SaveSlotName = TEXT("MO56_Default");
        static constexpr int32 SaveUserIndex = 0;

        UPROPERTY()
        TObjectPtr<UMO56SaveGame> CurrentSaveGame = nullptr;

        TMap<FGuid, TWeakObjectPtr<UInventoryComponent>> RegisteredInventories;
        TSet<FGuid> PlayerInventoryIds;
        TMap<FGuid, TWeakObjectPtr<AItemPickup>> TrackedPickups;
        TMap<FGuid, FName> PickupToLevelMap;

        TMap<UWorld*, FDelegateHandle> WorldSpawnHandles;
        FDelegateHandle PostWorldInitHandle;
        FDelegateHandle WorldCleanupHandle;

        bool bIsApplyingSave = false;

private:
        void HandlePostWorldInit(UWorld* World, const UWorld::InitializationValues IVS);
        void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
        void HandleActorSpawned(AActor* Actor);

        UFUNCTION()
        void HandlePickupSettled(AItemPickup* Pickup);

        UFUNCTION()
        void HandlePickupDestroyed(AItemPickup* Pickup);

        void LoadOrCreateSaveGame();
        void ApplySaveToInventories();
        void ApplySaveToWorld(UWorld* World);
        void RefreshInventorySaveData();
        void RefreshTrackedPickups();

        FName ResolveLevelName(const AActor& Actor) const;
        FName ResolveLevelName(const UWorld& World) const;

        void BindPickupDelegates(AItemPickup& Pickup);
        void UnbindPickupDelegates(AItemPickup& Pickup);

        void ApplyPlayerTransforms();
        bool ApplyLoadedSaveGame(UMO56SaveGame* LoadedSave);

        void SanitizeLoadedSave(UMO56SaveGame& Save);

        UFUNCTION()
        void HandleInventoryComponentUpdated();
};

