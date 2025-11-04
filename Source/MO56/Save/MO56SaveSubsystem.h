// Implementation: Game instance subsystem that serializes inventories and pickups. Register
// pawn/container inventory components in BeginPlay so their state is captured; call
// SaveGame/LoadGame from UI or gameplay logic. Extend this subsystem when persisting new
// systems such as quests or crafting.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/MO56SaveGame.h"
#include "Save/MO56SaveTypes.h"
#include "MO56PlayerController.h"
#include "Delegates/Delegate.h"
#include "TimerManager.h"
#include "MO56SaveSubsystem.generated.h"

class AItemPickup;
class UInventoryComponent;
class USkillSystemComponent;
class AMO56PlayerController;
class AMO56Character;
class APawn;
class APlayerController;
class UMOPersistentPawnComponent;
class UWorld;

enum class ERegisterPlayerCharacterContext : uint8
{
        FirstAssociation,
        PossessionSwitch
};

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

        /** Identifier of the save entry. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
        FGuid SaveId;
};

/**
 * Centralized save-game subsystem that persists inventory and world pickup data.
 *
 * Editor Implementation Guide:
 * 1. Add UMO56SaveSubsystem to your GameInstance (Project Settings > Maps & Modes > Game Instance) so it initializes on launch.
 * 2. Expose Blueprint functions that call SaveGame/LoadGame/SetActiveSaveSlot for menus created in UMG.
 * 3. Ensure playable pawns call RegisterInventoryComponent/RegisterSkillComponent in BeginPlay for persistence.
 * 4. Hook NotifyPlayerControllerReady/RegisterPlayerCharacter from PlayerController/Character Blueprints after possession.
 * 5. Extend RegisterWorldPickup/TrackedPickups when introducing new persistent actors (structures, quest items, etc.).
 */

UENUM(BlueprintType)
enum class EMO56InventoryOwner : uint8
{
        Character,
        Container,
        World,
        Unknown
};

UCLASS()
class MO56_API UMO56SaveSubsystem : public UGameInstanceSubsystem
{
        GENERATED_BODY()

public:
        UMO56SaveSubsystem();

        virtual void Initialize(FSubsystemCollectionBase& Collection) override;
        virtual void Deinitialize() override;

        UFUNCTION(BlueprintCallable, Category = "Save")
        TArray<FSaveIndexEntry> ListSaves(bool bRebuildFromDiskIfMissing = true);

        UFUNCTION(BlueprintCallable, Category = "Save")
        bool DoesSaveExist(const FGuid& SaveId) const;

        UFUNCTION(BlueprintCallable, Category = "Save")
        UMO56SaveGame* PeekSaveHeader(const FGuid& SaveId);

        UFUNCTION(BlueprintCallable, Category = "Save")
        void StartNewGame(const FString& LevelName = TEXT("M_TestLevel"));

        UFUNCTION(BlueprintCallable, Category = "Save")
        void LoadSave(const FGuid& SaveId);

        UFUNCTION(BlueprintCallable, Category = "Save")
        bool SaveCurrentGame();

        UFUNCTION(BlueprintCallable, Category = "Save")
        bool DeleteSave(const FGuid& SaveId);

        UFUNCTION(BlueprintCallable, Category = "Save")
        bool DeleteAllSaves(bool bAlsoDeleteIndex = true, bool bAlsoDeleteMenuSettings = false);

        /** Saves the current world and inventory state to disk. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        bool SaveGame(bool bForce = false);

        /** Loads the world and inventory state from disk. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        bool LoadGame();

        /** Loads save data from the specified slot name. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        bool LoadGameBySlot(const FString& SlotName, int32 UserIndex);

        /** Clears the current save data and resets runtime state. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        void ResetToNewGame();

        /** Creates a brand-new save slot populated with the current runtime state. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        FSaveGameSummary CreateNewSaveSlot();

        /** Updates the active slot used when calling SaveGame/LoadGame. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        void SetActiveSaveSlot(const FString& SlotName, int32 UserIndex);

        /** Registers an inventory component so its state is serialized. */
        void RegisterInventoryComponent(UInventoryComponent* InventoryComponent, EMO56InventoryOwner OwnerType, const FGuid& OwningId = FGuid());

        /** Stops tracking the specified inventory component. */
        void UnregisterInventoryComponent(UInventoryComponent* InventoryComponent);

        /** Registers a skill component for save serialization. */
        void RegisterSkillComponent(USkillSystemComponent* SkillComponent, const FGuid& OwningCharacterId = FGuid());

        /** Stops tracking the specified skill component. */
        void UnregisterSkillComponent(USkillSystemComponent* SkillComponent);

        /** Registers a pickup actor for persistence tracking. */
        void RegisterWorldPickup(AItemPickup* Pickup);

        /** Unregisters a pickup actor from persistence tracking. */
        void UnregisterWorldPickup(AItemPickup* Pickup);

        void AssignAndPossessPersistentPawn(APlayerController* PlayerController);

        bool BuildPossessablePawnList(APlayerController* ForPC, TArray<FMOPossessablePawnInfo>& Out) const;
        bool TryAssignAndPossess(APlayerController* PC, const FGuid& PawnId, FString& OutReason);

        /** Notifies the subsystem that a controller is ready and should be associated with save data. */
        void NotifyPlayerControllerReady(AMO56PlayerController* Controller);

        /** Registers a character pawn for persistence. */
        void RegisterCharacter(AMO56Character* Character);

        /** Removes a character pawn from persistence tracking. */
        void UnregisterCharacter(AMO56Character* Character);

        /** Associates a possessed character with its owning player for profile tracking. */
        void RegisterPlayerCharacter(AMO56Character* Character, AMO56PlayerController* Controller);

        /** Called when a skill component changes to refresh the active save. */
        void NotifySkillComponentUpdated(USkillSystemComponent* SkillComponent);

        /** Returns the active save game object. */
        UFUNCTION(BlueprintPure, Category = "Save")
        UMO56SaveGame* GetCurrentSaveGame() const { return CurrentSaveGame; }

        /** Returns summaries for all discoverable save games. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        TArray<FSaveGameSummary> GetAvailableSaveSummaries() const;

private:
        static constexpr const TCHAR* SaveSlotName = TEXT("MO56_Default");
        static constexpr int32 SaveUserIndex = 0;
        static constexpr const TCHAR* SaveIndexSlotName = TEXT("MO56_SaveIndex");

        UPROPERTY()
        TObjectPtr<UMO56SaveGame> CurrentSaveGame = nullptr;

        FString ActiveSaveSlotName;
        int32 ActiveSaveUserIndex = SaveUserIndex;

        UPROPERTY()
        TObjectPtr<UMO56SaveIndex> CachedSaveIndex = nullptr;

        UPROPERTY()
        TObjectPtr<UMO56SaveGame> PendingLoadedSave = nullptr;

        FGuid ActiveSaveId;
        FString PendingLevelName;
        bool bPendingApplyOnNextLevel = false;
        bool bPendingCreateNewSaveAfterTravel = false;
        bool bAppliedPendingSaveThisLevel = false;
        bool bIsRestoringWorld = false;

        UPROPERTY()
        TMap<FGuid, TWeakObjectPtr<UInventoryComponent>> RegisteredInventories;

        UPROPERTY()
        TMap<FGuid, TWeakObjectPtr<AItemPickup>> TrackedPickups;

        UPROPERTY()
        TMap<FGuid, FName> PickupToLevelMap;

        UPROPERTY()
        TMap<UInventoryComponent*, EMO56InventoryOwner> InventoryOwnerTypes;

        UPROPERTY()
        TMap<UInventoryComponent*, FGuid> InventoryOwnerIds;

        UPROPERTY()
        TMap<USkillSystemComponent*, FGuid> SkillComponentToCharacterId;

        UPROPERTY()
        TMap<FGuid, TWeakObjectPtr<USkillSystemComponent>> CharacterToSkillComponent;

        UPROPERTY()
        TMap<FGuid, TWeakObjectPtr<UInventoryComponent>> CharacterToInventoryComponent;

        UPROPERTY()
        TMap<FGuid, TWeakObjectPtr<AMO56PlayerController>> PlayerControllers;

        UPROPERTY()
        TMap<FGuid, TWeakObjectPtr<AMO56Character>> RegisteredCharacters;

        TSet<FGuid> PlayersWithEstablishedCharacters;

        TMap<UWorld*, FDelegateHandle> WorldSpawnHandles;
        FDelegateHandle WorldCleanupHandle;
        FDelegateHandle PostLoadMapHandle;

        bool bIsApplyingSave = false;
        bool bAutosavePending = false;
        FTimerHandle AutosaveTimerHandle;
        FTimerHandle PostLoadValidationTimerHandle;
        float AutosaveDelaySeconds = 0.2f;

        /** Map names that allow gameplay autosaves. */
        UPROPERTY(EditAnywhere, Category = "Save|Maps")
        TSet<FName> GameplayMapNames;

        /** Prefixes that indicate non-gameplay or menu maps. */
        UPROPERTY(EditAnywhere, Category = "Save|Maps")
        TArray<FString> NonGameplayMapPrefixes;

private:
        void HandlePostWorldInit(UWorld* World, const UWorld::InitializationValues IVS);
        void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
        void HandleActorSpawned(AActor* Actor);
        void HandlePostLoadMapWithWorld(UWorld* World);
        void HandleDeferredBeginPlay(TWeakObjectPtr<UWorld> WorldPtr);
        void RunServerPossessionPass(UWorld& World);
        void HandlePostLoadValidation(TWeakObjectPtr<UWorld> WorldPtr);

        void SpawnPersistentPawnsFromSave();
        APawn* FindPersistentPawnById(const FGuid& PawnId) const;
        bool FindUnassignedPlayerCandidate(FGuid& OutPawnId) const;
        void GatherPersistentPawnsForSave();


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
        bool ApplyPendingSave(UWorld& World);

        void SanitizeLoadedSave(UMO56SaveGame& Save);

        bool IsAuthoritative() const;
        FString GenerateUniqueSaveSlotName() const;

        UFUNCTION()
        void HandleInventoryComponentUpdated();

        void HandleSkillComponentRegistered(USkillSystemComponent* SkillComponent, const FGuid& CharacterId);
        void HandleInventoryRegistered(UInventoryComponent* InventoryComponent, EMO56InventoryOwner OwnerType, const FGuid& OwnerId);
        void SyncPlayerSaveData(const FGuid& PlayerId);
        void ApplyCharacterStateFromSave(const FGuid& CharacterId);
        void RefreshCharacterSaveData(const FGuid& CharacterId);
        void HandleAutosaveTimerElapsed();
        void SchedulePostPossessionValidation(UWorld& World);

        FString MakeSlotName(const FGuid& SaveId) const;
        FString GetSaveDir() const;
        bool IsSaveFileName(const FString& Name) const;
        bool WriteSave(const UMO56SaveGame* Data);

        bool IsRestoringWorld() const { return bIsRestoringWorld; }
        UMO56SaveGame* ReadSave(const FGuid& SaveId, bool bUpdateMetadata = true);
        void UpdateOrRebuildSaveIndex(bool bForceRebuild = false);
        void CacheSaveMetadata(UMO56SaveGame& SaveGame);
        bool IsMenuOrNonGameplayMap(const UWorld* World) const;
        bool CanAutosaveInWorld(const UWorld& World) const;
        bool IsGameplayMapName(const FName& MapName) const;
        bool IsMenuOrNonGameplayMapName(const FString& MapName) const;
};

