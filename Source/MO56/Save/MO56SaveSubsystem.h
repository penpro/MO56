#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/MO56SaveGame.h"
#include "MO56SaveSubsystem.generated.h"

class AItemPickup;
class UInventoryComponent;

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

private:
        static constexpr const TCHAR* SaveSlotName = TEXT("MO56_Default");
        static constexpr int32 SaveUserIndex = 0;

        UPROPERTY()
        TObjectPtr<UMO56SaveGame> CurrentSaveGame = nullptr;

        TMap<FGuid, TWeakObjectPtr<UInventoryComponent>> RegisteredInventories;
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

        bool IsPlayerInventoryComponent(const UInventoryComponent* InventoryComponent) const;

        FName ResolveLevelName(const AActor& Actor) const;
        FName ResolveLevelName(const UWorld& World) const;

        void BindPickupDelegates(AItemPickup& Pickup);
        void UnbindPickupDelegates(AItemPickup& Pickup);
};

