// Implementation: Registers inventories and world pickups, serializing them into the active
// UMO56SaveGame. Call SaveGame/LoadGame through this subsystem so multiplayer hosts persist
// shared state across sessions.
#include "Save/MO56SaveSubsystem.h"

#include "Algo/RemoveIf.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "InventoryComponent.h"
#include "ItemPickup.h"
#include "ItemData.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Templates/UnrealTemplate.h"
#include "MO56Character.h"
#include "Skills/SkillSystemComponent.h"
#include "HAL/FileManager.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMO56SaveSubsystem, Log, All);

UMO56SaveSubsystem::UMO56SaveSubsystem()
{
        ActiveSaveSlotName = SaveSlotName;
        ActiveSaveUserIndex = SaveUserIndex;
}

void UMO56SaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
        Super::Initialize(Collection);

        LoadOrCreateSaveGame();

        PostWorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UMO56SaveSubsystem::HandlePostWorldInit);
        WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UMO56SaveSubsystem::HandleWorldCleanup);
}

void UMO56SaveSubsystem::Deinitialize()
{
        Super::Deinitialize();

        if (PostWorldInitHandle.IsValid())
        {
                FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitHandle);
                PostWorldInitHandle.Reset();
        }

        if (WorldCleanupHandle.IsValid())
        {
                FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
                WorldCleanupHandle.Reset();
        }

        for (auto& Pair : WorldSpawnHandles)
        {
                if (UWorld* World = Pair.Key)
                {
                        World->RemoveOnActorSpawnedHandler(Pair.Value);
                }
        }

        WorldSpawnHandles.Empty();
        RegisteredInventories.Empty();
        TrackedPickups.Empty();
        PickupToLevelMap.Empty();
        CurrentSaveGame = nullptr;

        if (UWorld* World = GetWorld())
        {
                World->GetTimerManager().ClearTimer(AutosaveTimerHandle);
        }

        bAutosavePending = false;
}

bool UMO56SaveSubsystem::SaveGame()
{
        if (!IsAuthoritative())
        {
                UE_LOG(LogMO56SaveSubsystem, Verbose, TEXT("SaveGame skipped on non-authority instance."));
                return false;
        }

        if (!CurrentSaveGame)
        {
                LoadOrCreateSaveGame();
        }

        if (UWorld* World = GetWorld())
        {
                World->GetTimerManager().ClearTimer(AutosaveTimerHandle);
        }

        bAutosavePending = false;

        RefreshInventorySaveData();
        RefreshTrackedPickups();

        if (!CurrentSaveGame)
        {
                return false;
        }

        const FDateTime NowUtc = FDateTime::UtcNow();
        if (CurrentSaveGame->InitialSaveTimestamp.GetTicks() == 0)
        {
                CurrentSaveGame->InitialSaveTimestamp = NowUtc;
        }

        CurrentSaveGame->LastSaveTimestamp = NowUtc;

        if (UWorld* World = GetWorld())
        {
                CurrentSaveGame->TotalPlayTimeSeconds = World->GetTimeSeconds();
                const FName LevelName = ResolveLevelName(*World);
                if (!LevelName.IsNone())
                {
                        CurrentSaveGame->LastLevelName = LevelName;
                }
        }

        const FString SlotName = ActiveSaveSlotName.IsEmpty() ? SaveSlotName : ActiveSaveSlotName;
        const bool bSaved = UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SlotName, ActiveSaveUserIndex);
        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("SaveGame: Slot=%s Result=%s"), *SlotName, bSaved ? TEXT("Success") : TEXT("Failure"));
        return bSaved;
}

bool UMO56SaveSubsystem::LoadGame()
{
        const FString SlotName = ActiveSaveSlotName.IsEmpty() ? SaveSlotName : ActiveSaveSlotName;
        return LoadGameBySlot(SlotName, ActiveSaveUserIndex);
}

void UMO56SaveSubsystem::ResetToNewGame()
{
        if (!IsAuthoritative())
        {
                UE_LOG(LogMO56SaveSubsystem, Verbose, TEXT("ResetToNewGame ignored on non-authority instance."));
                return;
        }

        const FString SlotName = ActiveSaveSlotName.IsEmpty() ? SaveSlotName : ActiveSaveSlotName;
        UGameplayStatics::DeleteGameInSlot(SlotName, ActiveSaveUserIndex);

        CurrentSaveGame = NewObject<UMO56SaveGame>(this);
        const FDateTime NowUtc = FDateTime::UtcNow();
        CurrentSaveGame->InitialSaveTimestamp = NowUtc;
        CurrentSaveGame->LastSaveTimestamp = NowUtc;
        CurrentSaveGame->PlayerInventoryIds.Empty();
        CurrentSaveGame->CharacterStates.Empty();
        CurrentSaveGame->InventoryStates.Empty();
        CurrentSaveGame->LevelStates.Empty();
        CurrentSaveGame->PlayerStates.Empty();

        if (UWorld* World = GetWorld())
        {
                World->GetTimerManager().ClearTimer(AutosaveTimerHandle);
        }

        bAutosavePending = false;

        TArray<TWeakObjectPtr<AItemPickup>> PickupsToDestroy;
        PickupsToDestroy.Reserve(TrackedPickups.Num());

        for (const auto& Pair : TrackedPickups)
        {
                if (AItemPickup* Pickup = Pair.Value.Get())
                {
                        if (Pickup->WasSpawnedFromInventory())
                        {
                                PickupsToDestroy.Add(Pair.Value);
                        }
                }
        }

        for (const TWeakObjectPtr<AItemPickup>& PickupPtr : PickupsToDestroy)
        {
                if (AItemPickup* Pickup = PickupPtr.Get())
                {
                        Pickup->Destroy();
                }
        }

        ApplySaveToInventories();

        if (UWorld* World = GetWorld())
        {
                ApplySaveToWorld(World);
        }

        TrackedPickups.Empty();
        PickupToLevelMap.Empty();
        InventoryOwnerTypes.Empty();
        InventoryOwnerIds.Empty();
        SkillComponentToCharacterId.Empty();
        CharacterToSkillComponent.Empty();
        CharacterToInventoryComponent.Empty();
        PlayerControllers.Empty();
        RegisteredCharacters.Empty();

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("ResetToNewGame: save data cleared."));
}

FSaveGameSummary UMO56SaveSubsystem::CreateNewSaveSlot()
{
        FSaveGameSummary Summary;

        if (!IsAuthoritative())
        {
                UE_LOG(LogMO56SaveSubsystem, Verbose, TEXT("CreateNewSaveSlot ignored on non-authority instance."));
                return Summary;
        }

        if (!CurrentSaveGame)
        {
                LoadOrCreateSaveGame();
        }

        if (!CurrentSaveGame)
        {
                return Summary;
        }

        SaveGame();

        const FString NewSlotName = GenerateUniqueSaveSlotName();
        if (NewSlotName.IsEmpty())
        {
                return Summary;
        }

        const bool bSaved = UGameplayStatics::SaveGameToSlot(CurrentSaveGame, NewSlotName, ActiveSaveUserIndex);
        if (!bSaved)
        {
                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("CreateNewSaveSlot: failed to persist slot %s"), *NewSlotName);
                return Summary;
        }

        Summary.SlotName = NewSlotName;
        Summary.UserIndex = ActiveSaveUserIndex;
        Summary.InitialSaveTimestamp = CurrentSaveGame->InitialSaveTimestamp;
        Summary.LastSaveTimestamp = CurrentSaveGame->LastSaveTimestamp;
        Summary.TotalPlayTimeSeconds = CurrentSaveGame->TotalPlayTimeSeconds;
        Summary.LastLevelName = CurrentSaveGame->LastLevelName;
        Summary.InventoryCount = CurrentSaveGame->InventoryStates.Num();

        ActiveSaveSlotName = NewSlotName;

        return Summary;
}

void UMO56SaveSubsystem::SetActiveSaveSlot(const FString& SlotName, int32 UserIndex)
{
        ActiveSaveSlotName = SlotName.IsEmpty() ? SaveSlotName : SlotName;
        ActiveSaveUserIndex = UserIndex >= 0 ? UserIndex : SaveUserIndex;

        CurrentSaveGame = nullptr;
        LoadOrCreateSaveGame();
}

void UMO56SaveSubsystem::RegisterInventoryComponent(UInventoryComponent* InventoryComponent, EMO56InventoryOwner OwnerType, const FGuid& OwningId)
{
        if (!InventoryComponent)
        {
                return;
        }

        if (!IsAuthoritative())
        {
                return;
        }

        HandleInventoryRegistered(InventoryComponent, OwnerType, OwningId);
}

void UMO56SaveSubsystem::UnregisterInventoryComponent(UInventoryComponent* InventoryComponent)
{
        if (!InventoryComponent)
        {
                return;
        }

        if (!IsAuthoritative())
        {
                return;
        }

        const FGuid InventoryId = InventoryComponent->GetPersistentId();
        const EMO56InventoryOwner* OwnerTypePtr = InventoryOwnerTypes.Find(InventoryComponent);
        const FGuid OwnerId = InventoryOwnerIds.FindRef(InventoryComponent);

        if (CurrentSaveGame)
        {
                FInventorySaveData& SaveData = CurrentSaveGame->InventoryStates.FindOrAdd(InventoryId);
                InventoryComponent->WriteToSaveData(SaveData);

                if (OwnerTypePtr && *OwnerTypePtr == EMO56InventoryOwner::Character && OwnerId.IsValid())
                {
                        RefreshCharacterSaveData(OwnerId);
                }
        }

        RegisteredInventories.Remove(InventoryId);

        if (OwnerTypePtr && *OwnerTypePtr == EMO56InventoryOwner::Character && OwnerId.IsValid())
        {
                if (TWeakObjectPtr<UInventoryComponent>* Entry = CharacterToInventoryComponent.Find(OwnerId))
                {
                        if (Entry->Get() == InventoryComponent)
                        {
                                CharacterToInventoryComponent.Remove(OwnerId);
                        }
                }
        }

        InventoryOwnerTypes.Remove(InventoryComponent);
        InventoryOwnerIds.Remove(InventoryComponent);

        InventoryComponent->OnInventoryUpdated.RemoveDynamic(this, &UMO56SaveSubsystem::HandleInventoryComponentUpdated);
}

void UMO56SaveSubsystem::RegisterSkillComponent(USkillSystemComponent* SkillComponent, const FGuid& OwningCharacterId)
{
        if (!SkillComponent || !IsAuthoritative())
        {
                return;
        }

        HandleSkillComponentRegistered(SkillComponent, OwningCharacterId);
}

void UMO56SaveSubsystem::UnregisterSkillComponent(USkillSystemComponent* SkillComponent)
{
        if (!SkillComponent || !IsAuthoritative())
        {
                return;
        }

        if (const FGuid* CharacterIdPtr = SkillComponentToCharacterId.Find(SkillComponent))
        {
                if (CharacterIdPtr->IsValid())
                {
                        RefreshCharacterSaveData(*CharacterIdPtr);
                        CharacterToSkillComponent.Remove(*CharacterIdPtr);
                }
        }

        SkillComponentToCharacterId.Remove(SkillComponent);
}

void UMO56SaveSubsystem::NotifyPlayerControllerReady(AMO56PlayerController* Controller)
{
        if (!Controller || !IsAuthoritative())
        {
                return;
        }

        FGuid PlayerId = Controller->GetPlayerSaveId();
        APlayerState* PlayerState = Controller->GetPlayerState<APlayerState>();
        const int32 ControllerId = PlayerState ? PlayerState->GetPlayerId() : INDEX_NONE;

        if (!PlayerId.IsValid() && CurrentSaveGame)
        {
                        for (auto& Pair : CurrentSaveGame->PlayerStates)
                        {
                                if (Pair.Value.ControllerId == ControllerId && Pair.Value.PlayerId.IsValid())
                                {
                                        PlayerId = Pair.Value.PlayerId;
                                        break;
                                }
                        }
        }

        if (!PlayerId.IsValid())
        {
                PlayerId = FGuid::NewGuid();
        }

        Controller->SetPlayerSaveId(PlayerId);
        PlayerControllers.FindOrAdd(PlayerId) = Controller;

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("NotifyPlayerControllerReady: PlayerId=%s ControllerId=%d"), *PlayerId.ToString(), ControllerId);

        if (!CurrentSaveGame)
        {
                LoadOrCreateSaveGame();
        }

        if (CurrentSaveGame)
        {
                FPlayerSaveData& PlayerData = CurrentSaveGame->PlayerStates.FindOrAdd(PlayerId);
                PlayerData.PlayerId = PlayerId;
                PlayerData.ControllerId = ControllerId;
                if (PlayerState)
                {
                        PlayerData.PlayerName = PlayerState->GetPlayerName();
                }
        }

}

void UMO56SaveSubsystem::RegisterCharacter(AMO56Character* Character)
{
        if (!Character || !IsAuthoritative())
        {
                return;
        }

        const FGuid CharacterId = Character->GetCharacterId();
        if (!CharacterId.IsValid())
        {
                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("RegisterCharacter: Character %s has invalid CharacterId."), *GetNameSafe(Character));
                return;
        }

        RegisteredCharacters.FindOrAdd(CharacterId) = Character;

        if (!CurrentSaveGame)
        {
                LoadOrCreateSaveGame();
        }

        if (CurrentSaveGame)
        {
                FCharacterSaveData& CharacterData = CurrentSaveGame->CharacterStates.FindOrAdd(CharacterId);
                CharacterData.CharacterId = CharacterId;
        }

        const FGuid InventoryId = Character->GetInventoryComponent() ? Character->GetInventoryComponent()->GetPersistentId() : FGuid();
        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("RegisterCharacter: CharId=%s InvId=%s"), *CharacterId.ToString(), InventoryId.IsValid() ? *InventoryId.ToString() : TEXT("None"));

        ApplyCharacterStateFromSave(CharacterId);
}

void UMO56SaveSubsystem::UnregisterCharacter(AMO56Character* Character)
{
        if (!Character || !IsAuthoritative())
        {
                return;
        }

        const FGuid CharacterId = Character->GetCharacterId();
        if (!CharacterId.IsValid())
        {
                return;
        }

        if (CurrentSaveGame)
        {
                RefreshCharacterSaveData(CharacterId);
        }

        CharacterToInventoryComponent.Remove(CharacterId);
        CharacterToSkillComponent.Remove(CharacterId);
        RegisteredCharacters.Remove(CharacterId);
}

void UMO56SaveSubsystem::RegisterPlayerCharacter(AMO56Character* Character, AMO56PlayerController* Controller)
{
        if (!Character || !IsAuthoritative())
        {
                return;
        }

        const FGuid PlayerId = Controller ? Controller->GetPlayerSaveId() : FGuid();
        if (!PlayerId.IsValid())
        {
                return;
        }

        PlayerControllers.FindOrAdd(PlayerId) = Controller;

        const FGuid CharacterId = Character->GetCharacterId();
        const FGuid InventoryId = Character->GetInventoryComponent() ? Character->GetInventoryComponent()->GetPersistentId() : FGuid();
        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("Possess: PC=%s Pawn=%s CharId=%s InvId=%s (no data moved)"), *GetNameSafe(Controller), *GetNameSafe(Character), CharacterId.IsValid() ? *CharacterId.ToString() : TEXT("None"), InventoryId.IsValid() ? *InventoryId.ToString() : TEXT("None"));

        if (CurrentSaveGame && CharacterId.IsValid())
        {
                FCharacterSaveData* CharacterData = CurrentSaveGame->CharacterStates.Find(CharacterId);
                const bool bNeedsMigration = !CharacterData || !CharacterData->InventoryId.IsValid() || CurrentSaveGame->PlayerInventoryIds.Contains(PlayerId);
                if (bNeedsMigration)
                {
                        if (const FGuid* LegacyInventoryId = CurrentSaveGame->PlayerInventoryIds.Find(PlayerId))
                        {
                                FCharacterSaveData& CharacterDataRef = CurrentSaveGame->CharacterStates.FindOrAdd(CharacterId);
                                CharacterDataRef.CharacterId = CharacterId;
                                CharacterDataRef.InventoryId = *LegacyInventoryId;

                                if (const FPlayerSaveData* PlayerData = CurrentSaveGame->PlayerStates.Find(PlayerId))
                                {
                                        CharacterDataRef.SkillState = PlayerData->SkillState;
                                        CharacterDataRef.Transform = PlayerData->Transform;
                                }

                                CurrentSaveGame->PlayerInventoryIds.Remove(PlayerId);
                                ApplyCharacterStateFromSave(CharacterId);
                        }
                }
        }

        SyncPlayerSaveData(PlayerId);
}

void UMO56SaveSubsystem::NotifySkillComponentUpdated(USkillSystemComponent* SkillComponent)
{
        if (!SkillComponent || !IsAuthoritative() || bIsApplyingSave)
        {
                return;
        }

        if (const FGuid* CharacterIdPtr = SkillComponentToCharacterId.Find(SkillComponent))
        {
                if (CharacterIdPtr->IsValid())
                {
                        RefreshCharacterSaveData(*CharacterIdPtr);
                }
        }
}

void UMO56SaveSubsystem::RegisterWorldPickup(AItemPickup* Pickup)
{
        if (!Pickup)
        {
                return;
        }

        if (!IsAuthoritative())
        {
                return;
        }

        if (!Pickup->GetPersistentId().IsValid())
        {
                Pickup->SetPersistentId(FGuid::NewGuid());
        }

        BindPickupDelegates(*Pickup);

        const FGuid PickupId = Pickup->GetPersistentId();
        const FName LevelName = ResolveLevelName(*Pickup);
        PickupToLevelMap.Add(PickupId, LevelName);

        if (!CurrentSaveGame)
        {
                LoadOrCreateSaveGame();
        }

        if (!CurrentSaveGame)
        {
                return;
        }

        if (!LevelName.IsNone())
        {
                if (FLevelWorldState* LevelState = CurrentSaveGame->LevelStates.Find(LevelName))
                {
                        if (!bIsApplyingSave && LevelState->RemovedPickupIds.Contains(PickupId))
                        {
                                Pickup->Destroy();
                                return;
                        }

                        if (FWorldItemSaveData* SavedData = LevelState->DroppedItems.FindByPredicate([&](const FWorldItemSaveData& Entry)
                        {
                                return Entry.PickupId == PickupId;
                        }))
                        {
                                TrackedPickups.Add(PickupId, Pickup);

                                if (UItemData* ItemData = Cast<UItemData>(SavedData->ItemPath.TryLoad()))
                                {
                                        Pickup->SetItem(ItemData);
                                }

                                Pickup->SetQuantity(SavedData->Quantity);
                                Pickup->SetWasSpawnedFromInventory(SavedData->bSpawnedFromInventory);
                                Pickup->SetActorTransform(SavedData->Transform);
                        }
                }
        }
}

void UMO56SaveSubsystem::UnregisterWorldPickup(AItemPickup* Pickup)
{
        if (!Pickup)
        {
                return;
        }

        if (!IsAuthoritative())
        {
                return;
        }

        UnbindPickupDelegates(*Pickup);

        const FGuid PickupId = Pickup->GetPersistentId();
        TrackedPickups.Remove(PickupId);
        PickupToLevelMap.Remove(PickupId);
}

void UMO56SaveSubsystem::HandlePostWorldInit(UWorld* World, const UWorld::InitializationValues IVS)
{
        if (!World || !World->IsGameWorld())
        {
                return;
        }

        if (!IsAuthoritative() || World->GetNetMode() == NM_Client)
        {
                return;
        }

        FDelegateHandle SpawnHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UMO56SaveSubsystem::HandleActorSpawned));
        WorldSpawnHandles.Add(World, SpawnHandle);

        for (TActorIterator<AItemPickup> It(World); It; ++It)
        {
                RegisterWorldPickup(*It);
        }

        ApplySaveToWorld(World);

        if (World->GetNetMode() != NM_Client)
        {
                SaveGame();
        }
}

void UMO56SaveSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
        if (!World)
        {
                return;
        }

        if (FDelegateHandle* Handle = WorldSpawnHandles.Find(World))
        {
                World->RemoveOnActorSpawnedHandler(*Handle);
                WorldSpawnHandles.Remove(World);
        }
}

void UMO56SaveSubsystem::HandleActorSpawned(AActor* Actor)
{
        if (!IsAuthoritative())
        {
                return;
        }

        if (AItemPickup* Pickup = Cast<AItemPickup>(Actor))
        {
                RegisterWorldPickup(Pickup);
        }
}

void UMO56SaveSubsystem::HandlePickupSettled(AItemPickup* Pickup)
{
        if (!Pickup || !CurrentSaveGame)
        {
                return;
        }

        if (!IsAuthoritative())
        {
                return;
        }

        if (!Pickup->GetPersistentId().IsValid())
        {
                Pickup->SetPersistentId(FGuid::NewGuid());
        }

        const FGuid PickupId = Pickup->GetPersistentId();
        const FName LevelName = ResolveLevelName(*Pickup);

        if (LevelName.IsNone())
        {
                return;
        }

        FLevelWorldState& LevelState = CurrentSaveGame->LevelStates.FindOrAdd(LevelName);
        FWorldItemSaveData* Entry = LevelState.DroppedItems.FindByPredicate([&](const FWorldItemSaveData& Data)
        {
                return Data.PickupId == PickupId;
        });

        if (!Entry)
        {
                Entry = &LevelState.DroppedItems.AddDefaulted_GetRef();
        }

        Entry->PickupId = PickupId;
        Entry->ItemPath = Pickup->GetItem() ? FSoftObjectPath(Pickup->GetItem()) : FSoftObjectPath();
        Entry->PickupClass = Pickup->GetClass();
        Entry->Transform = Pickup->GetActorTransform();
        Entry->Quantity = Pickup->GetQuantity();
        Entry->bSpawnedFromInventory = Pickup->WasSpawnedFromInventory();

        LevelState.RemovedPickupIds.Remove(PickupId);

        TrackedPickups.Add(PickupId, Pickup);
        PickupToLevelMap.Add(PickupId, LevelName);
}

void UMO56SaveSubsystem::HandlePickupDestroyed(AItemPickup* Pickup)
{
        if (!Pickup)
        {
                return;
        }

        UnbindPickupDelegates(*Pickup);

        if (!IsAuthoritative())
        {
                return;
        }

        if (!CurrentSaveGame)
        {
                return;
        }

        const FGuid PickupId = Pickup->GetPersistentId();
        FName LevelName = ResolveLevelName(*Pickup);

        if (LevelName.IsNone())
        {
                if (const FName* StoredLevelName = PickupToLevelMap.Find(PickupId))
                {
                        LevelName = *StoredLevelName;
                }
        }

        TrackedPickups.Remove(PickupId);
        PickupToLevelMap.Remove(PickupId);

        if (bIsApplyingSave)
        {
                return;
        }

        if (LevelName.IsNone())
        {
                return;
        }

        FLevelWorldState& LevelState = CurrentSaveGame->LevelStates.FindOrAdd(LevelName);
        LevelState.DroppedItems.RemoveAll([&](const FWorldItemSaveData& Data)
        {
                return Data.PickupId == PickupId;
        });

        if (!Pickup->WasSpawnedFromInventory())
        {
                LevelState.RemovedPickupIds.Add(PickupId);
        }
        else
        {
                LevelState.RemovedPickupIds.Remove(PickupId);
        }
}

void UMO56SaveSubsystem::LoadOrCreateSaveGame()
{
        if (CurrentSaveGame)
        {
                return;
        }

        const FString SlotName = ActiveSaveSlotName.IsEmpty() ? SaveSlotName : ActiveSaveSlotName;
        const int32 UserIndex = ActiveSaveUserIndex;

        if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex))
        {
                CurrentSaveGame = Cast<UMO56SaveGame>(Loaded);
                if (CurrentSaveGame)
                {
                        SanitizeLoadedSave(*CurrentSaveGame);
                }
        }

        if (!CurrentSaveGame)
        {
                CurrentSaveGame = NewObject<UMO56SaveGame>(this);
                const FDateTime NowUtc = FDateTime::UtcNow();
                CurrentSaveGame->InitialSaveTimestamp = NowUtc;
                CurrentSaveGame->LastSaveTimestamp = NowUtc;
        }
}

void UMO56SaveSubsystem::ApplySaveToInventories()
{
        if (!CurrentSaveGame)
        {
                return;
        }

        if (!IsAuthoritative())
        {
                return;
        }

        TGuardValue<bool> ApplyingGuard(bIsApplyingSave, true);

        for (auto It = RegisteredInventories.CreateIterator(); It; ++It)
        {
                const FGuid InventoryId = It.Key();
                if (UInventoryComponent* Inventory = It.Value().Get())
                {
                        if (const FInventorySaveData* Data = CurrentSaveGame->InventoryStates.Find(InventoryId))
                        {
                                Inventory->ReadFromSaveData(*Data);
                        }
                        else
                        {
                                FInventorySaveData EmptyData;
                                Inventory->ReadFromSaveData(EmptyData);
                        }
                }
                else
                {
                        It.RemoveCurrent();
                }
        }
}

void UMO56SaveSubsystem::ApplySaveToWorld(UWorld* World)
{
        if (!World || !CurrentSaveGame)
        {
                return;
        }

        if (!IsAuthoritative())
        {
                return;
        }

        const FName LevelName = ResolveLevelName(*World);
        if (LevelName.IsNone())
        {
                return;
        }

        FLevelWorldState* LevelState = CurrentSaveGame->LevelStates.Find(LevelName);
        if (!LevelState)
        {
                return;
        }

        TGuardValue<bool> ApplyingGuard(bIsApplyingSave, true);

        TSet<FGuid> PendingIds;
        for (const FWorldItemSaveData& Entry : LevelState->DroppedItems)
        {
                PendingIds.Add(Entry.PickupId);
        }

        for (TActorIterator<AItemPickup> It(World); It; ++It)
        {
                AItemPickup* Pickup = *It;
                if (!Pickup)
                {
                        continue;
                }

                const FGuid PickupId = Pickup->GetPersistentId();
                if (!PickupId.IsValid())
                {
                        continue;
                }

                if (LevelState->RemovedPickupIds.Contains(PickupId))
                {
                        Pickup->Destroy();
                        continue;
                }

                if (const FWorldItemSaveData* SavedData = LevelState->DroppedItems.FindByPredicate([&](const FWorldItemSaveData& Data)
                {
                        return Data.PickupId == PickupId;
                }))
                {
                        if (UItemData* ItemData = Cast<UItemData>(SavedData->ItemPath.TryLoad()))
                        {
                                Pickup->SetItem(ItemData);
                        }

                        Pickup->SetQuantity(SavedData->Quantity);
                        Pickup->SetWasSpawnedFromInventory(SavedData->bSpawnedFromInventory);
                        Pickup->SetActorTransform(SavedData->Transform);

                        PendingIds.Remove(PickupId);
                }
        }

        for (const FWorldItemSaveData& SavedData : LevelState->DroppedItems)
        {
                if (!PendingIds.Contains(SavedData.PickupId))
                {
                        continue;
                }

                UItemData* ItemData = Cast<UItemData>(SavedData.ItemPath.TryLoad());
                if (!ItemData)
                {
                        continue;
                }

                UClass* PickupClass = SavedData.PickupClass.LoadSynchronous();
                if (!PickupClass)
                {
                        PickupClass = AItemPickup::StaticClass();
                }

                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

                const FVector Location = SavedData.Transform.GetLocation();
                const FRotator Rotation = SavedData.Transform.GetRotation().Rotator();

                AItemPickup* SpawnedPickup = World->SpawnActor<AItemPickup>(PickupClass, Location, Rotation, SpawnParams);
                if (!SpawnedPickup)
                {
                        continue;
                }

                SpawnedPickup->SetPersistentId(SavedData.PickupId);
                SpawnedPickup->SetItem(ItemData);
                SpawnedPickup->SetQuantity(SavedData.Quantity);
                SpawnedPickup->SetWasSpawnedFromInventory(SavedData.bSpawnedFromInventory);
                SpawnedPickup->SetActorTransform(SavedData.Transform);
        }
}

void UMO56SaveSubsystem::RefreshInventorySaveData()
{
        if (!CurrentSaveGame)
        {
                return;
        }

        if (!IsAuthoritative())
        {
                return;
        }

        for (auto It = RegisteredInventories.CreateIterator(); It; ++It)
        {
                const FGuid InventoryId = It.Key();
                UInventoryComponent* Inventory = It.Value().Get();
                if (!Inventory)
                {
                        CurrentSaveGame->InventoryStates.Remove(InventoryId);
                        It.RemoveCurrent();
                        continue;
                }

                FInventorySaveData& SaveData = CurrentSaveGame->InventoryStates.FindOrAdd(InventoryId);
                Inventory->WriteToSaveData(SaveData);

                if (const EMO56InventoryOwner* OwnerTypePtr = InventoryOwnerTypes.Find(Inventory))
                {
                        if (*OwnerTypePtr == EMO56InventoryOwner::Character)
                        {
                                const FGuid OwnerId = InventoryOwnerIds.FindRef(Inventory);
                                if (OwnerId.IsValid())
                                {
                                        RefreshCharacterSaveData(OwnerId);
                                }
                        }
                }
        }

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("RefreshInventorySaveData: Characters=%d Inventories=%d"), CurrentSaveGame->CharacterStates.Num(), CurrentSaveGame->InventoryStates.Num());
}

void UMO56SaveSubsystem::RefreshTrackedPickups()
{
        if (!CurrentSaveGame)
        {
                return;
        }

        if (!IsAuthoritative())
        {
                return;
        }

        for (auto It = TrackedPickups.CreateIterator(); It; ++It)
        {
                const FGuid PickupId = It.Key();
                AItemPickup* Pickup = It.Value().Get();
                if (!Pickup)
                {
                        It.RemoveCurrent();
                        continue;
                }

                FName LevelName = ResolveLevelName(*Pickup);
                if (LevelName.IsNone())
                {
                        if (const FName* StoredLevelName = PickupToLevelMap.Find(PickupId))
                        {
                                LevelName = *StoredLevelName;
                        }
                }

                if (LevelName.IsNone())
                {
                        continue;
                }

                FLevelWorldState& LevelState = CurrentSaveGame->LevelStates.FindOrAdd(LevelName);
                FWorldItemSaveData* Entry = LevelState.DroppedItems.FindByPredicate([&](const FWorldItemSaveData& Data)
                {
                        return Data.PickupId == PickupId;
                });

                if (!Entry)
                {
                        Entry = &LevelState.DroppedItems.AddDefaulted_GetRef();
                        Entry->PickupId = PickupId;
                }

                Entry->ItemPath = Pickup->GetItem() ? FSoftObjectPath(Pickup->GetItem()) : FSoftObjectPath();
                Entry->PickupClass = Pickup->GetClass();
                Entry->Transform = Pickup->GetActorTransform();
                Entry->Quantity = Pickup->GetQuantity();
                Entry->bSpawnedFromInventory = Pickup->WasSpawnedFromInventory();
        }
}

FName UMO56SaveSubsystem::ResolveLevelName(const AActor& Actor) const
{
        if (const ULevel* Level = Actor.GetLevel())
        {
                if (const UPackage* Package = Level->GetOutermost())
                {
                        return Package->GetFName();
                }
        }

        if (const UWorld* World = Actor.GetWorld())
        {
                return ResolveLevelName(*World);
        }

        return NAME_None;
}

FName UMO56SaveSubsystem::ResolveLevelName(const UWorld& World) const
{
        if (const UPackage* Package = World.GetOutermost())
        {
                return Package->GetFName();
        }

        return NAME_None;
}

void UMO56SaveSubsystem::BindPickupDelegates(AItemPickup& Pickup)
{
        Pickup.OnDropSettled.AddUniqueDynamic(this, &UMO56SaveSubsystem::HandlePickupSettled);
        Pickup.OnPickupDestroyed.AddUniqueDynamic(this, &UMO56SaveSubsystem::HandlePickupDestroyed);
}

void UMO56SaveSubsystem::UnbindPickupDelegates(AItemPickup& Pickup)
{
        Pickup.OnDropSettled.RemoveDynamic(this, &UMO56SaveSubsystem::HandlePickupSettled);
        Pickup.OnPickupDestroyed.RemoveDynamic(this, &UMO56SaveSubsystem::HandlePickupDestroyed);
}

bool UMO56SaveSubsystem::ApplyLoadedSaveGame(UMO56SaveGame* LoadedSave)
{
        if (!LoadedSave)
        {
                return false;
        }

        if (!IsAuthoritative())
        {
                return false;
        }

        SanitizeLoadedSave(*LoadedSave);
        CurrentSaveGame = LoadedSave;

        ApplySaveToInventories();

        if (UWorld* World = GetWorld())
        {
                ApplySaveToWorld(World);
        }

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("LoadGame: success"));
        return true;
}

void UMO56SaveSubsystem::SanitizeLoadedSave(UMO56SaveGame& Save)
{
        for (auto It = Save.CharacterStates.CreateIterator(); It; ++It)
        {
                FCharacterSaveData& CharacterData = It.Value();
                if (!CharacterData.CharacterId.IsValid())
                {
                        It.RemoveCurrent();
                        continue;
                }

                if (CharacterData.InventoryId.IsValid() && !Save.InventoryStates.Contains(CharacterData.InventoryId))
                {
                        CharacterData.InventoryId.Invalidate();
                }
        }

        for (auto It = Save.PlayerInventoryIds.CreateIterator(); It; ++It)
        {
                const FGuid InventoryId = It.Value();
                if (!InventoryId.IsValid() || !Save.InventoryStates.Contains(InventoryId))
                {
                        It.RemoveCurrent();
                }
        }

        if (Save.PlayerInventoryIds.Num() == 0 && Save.InventoryStates.Num() == 1)
        {
                const FGuid LegacyInventoryId = Save.InventoryStates.CreateConstIterator()->Key;
                if (LegacyInventoryId.IsValid())
                {
                        const FGuid PlayerId = Save.PlayerStates.Num() == 1 ? Save.PlayerStates.CreateConstIterator()->Key : FGuid::NewGuid();
                        Save.PlayerInventoryIds.Add(PlayerId, LegacyInventoryId);
                }
        }
}

bool UMO56SaveSubsystem::IsAuthoritative() const
{
        if (const UWorld* World = GetWorld())
        {
                return World->GetNetMode() != NM_Client;
        }

        return true;
}

FString UMO56SaveSubsystem::GenerateUniqueSaveSlotName() const
{
        const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
        const FString BaseName = FString::Printf(TEXT("MO56_Save_%s"), *Timestamp);

        FString Candidate = BaseName;
        int32 Counter = 1;

        while (UGameplayStatics::DoesSaveGameExist(Candidate, ActiveSaveUserIndex))
        {
                Candidate = FString::Printf(TEXT("%s_%d"), *BaseName, Counter++);
        }

        return Candidate;
}

bool UMO56SaveSubsystem::LoadGameBySlot(const FString& SlotName, int32 UserIndex)
{
        if (!IsAuthoritative())
        {
                UE_LOG(LogMO56SaveSubsystem, Verbose, TEXT("LoadGameBySlot ignored on non-authority instance."));
                return false;
        }

        if (SlotName.IsEmpty())
        {
                return false;
        }

        const int32 TargetUserIndex = UserIndex >= 0 ? UserIndex : ActiveSaveUserIndex;

        USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, TargetUserIndex);
        if (!Loaded)
        {
                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("LoadGame: no save present for slot %s"), *SlotName);
                return false;
        }

        if (UMO56SaveGame* LoadedSave = Cast<UMO56SaveGame>(Loaded))
        {
                ActiveSaveSlotName = SlotName;
                ActiveSaveUserIndex = TargetUserIndex;
                return ApplyLoadedSaveGame(LoadedSave);
        }

        UE_LOG(LogMO56SaveSubsystem, Error, TEXT("LoadGame: save slot %s contained an incompatible object"), *SlotName);
        return false;
}

TArray<FSaveGameSummary> UMO56SaveSubsystem::GetAvailableSaveSummaries() const
{
        TArray<FSaveGameSummary> Result;

        const FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
        if (!IFileManager::Get().DirectoryExists(*SaveDir))
        {
                return Result;
        }

        TArray<FString> SaveFiles;
        IFileManager::Get().FindFiles(SaveFiles, *(SaveDir / TEXT("*.sav")), true, false);

        for (const FString& SaveFile : SaveFiles)
        {
                const FString SlotName = FPaths::GetBaseFilename(SaveFile);
                if (SlotName.IsEmpty())
                {
                        continue;
                }

                FSaveGameSummary Summary;
                Summary.SlotName = SlotName;
                Summary.UserIndex = ActiveSaveUserIndex;

                if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, ActiveSaveUserIndex))
                {
                        if (const UMO56SaveGame* LoadedSave = Cast<UMO56SaveGame>(Loaded))
                        {
                                Summary.InitialSaveTimestamp = LoadedSave->InitialSaveTimestamp;
                                Summary.LastSaveTimestamp = LoadedSave->LastSaveTimestamp;
                                Summary.TotalPlayTimeSeconds = LoadedSave->TotalPlayTimeSeconds;
                                Summary.LastLevelName = LoadedSave->LastLevelName;
                                Summary.InventoryCount = LoadedSave->InventoryStates.Num();
                        }
                }

                Result.Add(MoveTemp(Summary));
        }

        return Result;
}

void UMO56SaveSubsystem::HandleInventoryComponentUpdated()
{
        if (bIsApplyingSave || !IsAuthoritative())
        {
                return;
        }

        if (bAutosavePending)
        {
                return;
        }

        if (UWorld* World = GetWorld())
        {
                FTimerManager& TimerManager = World->GetTimerManager();
                if (!TimerManager.IsTimerActive(AutosaveTimerHandle))
                {
                        TimerManager.SetTimer(AutosaveTimerHandle, this, &UMO56SaveSubsystem::HandleAutosaveTimerElapsed, AutosaveDelaySeconds, false);
                }

                bAutosavePending = true;
        }
}

void UMO56SaveSubsystem::HandleInventoryRegistered(UInventoryComponent* InventoryComponent, EMO56InventoryOwner OwnerType, const FGuid& OwnerId)
{
        if (!InventoryComponent)
        {
                return;
        }

        InventoryComponent->EnsurePersistentId();
        FGuid InventoryId = InventoryComponent->GetPersistentId();

        if (!CurrentSaveGame)
        {
                LoadOrCreateSaveGame();
        }

        if (OwnerType == EMO56InventoryOwner::Character && OwnerId.IsValid())
        {
                if (CurrentSaveGame)
                {
                        FCharacterSaveData& CharacterData = CurrentSaveGame->CharacterStates.FindOrAdd(OwnerId);
                        CharacterData.CharacterId = OwnerId;

                        if (CharacterData.InventoryId.IsValid() && CharacterData.InventoryId != InventoryId)
                        {
                                RegisteredInventories.Remove(InventoryId);
                                InventoryComponent->OverridePersistentId(CharacterData.InventoryId);
                                InventoryId = InventoryComponent->GetPersistentId();
                        }

                        if (!CharacterData.InventoryId.IsValid())
                        {
                                if (const FGuid* LegacyPlayerId = CurrentSaveGame->PlayerInventoryIds.FindKey(InventoryId))
                                {
                                        if (const FPlayerSaveData* LegacyPlayerData = CurrentSaveGame->PlayerStates.Find(*LegacyPlayerId))
                                        {
                                                CharacterData.SkillState = LegacyPlayerData->SkillState;
                                                CharacterData.Transform = LegacyPlayerData->Transform;
                                        }

                                        CurrentSaveGame->PlayerInventoryIds.Remove(*LegacyPlayerId);
                                }
                        }

                        CharacterData.InventoryId = InventoryComponent->GetPersistentId();
                }

                CharacterToInventoryComponent.FindOrAdd(OwnerId) = InventoryComponent;
                InventoryOwnerIds.FindOrAdd(InventoryComponent) = OwnerId;
        }

        InventoryOwnerTypes.FindOrAdd(InventoryComponent) = OwnerType;

        RegisteredInventories.Add(InventoryId, InventoryComponent);

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("RegisterInventoryComponent: InventoryId=%s OwnerType=%d OwnerId=%s"), *InventoryId.ToString(), static_cast<int32>(OwnerType), OwnerId.IsValid() ? *OwnerId.ToString() : TEXT("None"));

        InventoryComponent->OnInventoryUpdated.AddUniqueDynamic(this, &UMO56SaveSubsystem::HandleInventoryComponentUpdated);

        if (!CurrentSaveGame)
        {
                return;
        }

        if (const FInventorySaveData* SavedData = CurrentSaveGame->InventoryStates.Find(InventoryId))
        {
                TGuardValue<bool> ApplyingGuard(bIsApplyingSave, true);
                InventoryComponent->ReadFromSaveData(*SavedData);
        }

        if (OwnerType == EMO56InventoryOwner::Character && OwnerId.IsValid())
        {
                ApplyCharacterStateFromSave(OwnerId);
        }
}

void UMO56SaveSubsystem::HandleSkillComponentRegistered(USkillSystemComponent* SkillComponent, const FGuid& CharacterId)
{
        if (!SkillComponent)
        {
                return;
        }

        SkillComponentToCharacterId.FindOrAdd(SkillComponent) = CharacterId;

        if (!CharacterId.IsValid())
        {
                return;
        }

        CharacterToSkillComponent.FindOrAdd(CharacterId) = SkillComponent;

        if (!CurrentSaveGame)
        {
                LoadOrCreateSaveGame();
        }

        if (CurrentSaveGame)
        {
                if (FCharacterSaveData* CharacterData = CurrentSaveGame->CharacterStates.Find(CharacterId))
                {
                        TGuardValue<bool> ApplyingGuard(bIsApplyingSave, true);
                        SkillComponent->ReadFromSaveData(CharacterData->SkillState);
                }
        }

        ApplyCharacterStateFromSave(CharacterId);
}

void UMO56SaveSubsystem::HandleAutosaveTimerElapsed()
{
        bAutosavePending = false;

        if (!IsAuthoritative())
        {
                return;
        }

        RefreshInventorySaveData();
        SaveGame();
}

void UMO56SaveSubsystem::SyncPlayerSaveData(const FGuid& PlayerId)
{
        if (!CurrentSaveGame || !PlayerId.IsValid() || !IsAuthoritative())
        {
                return;
        }

        FPlayerSaveData& PlayerData = CurrentSaveGame->PlayerStates.FindOrAdd(PlayerId);
        PlayerData.PlayerId = PlayerId;

        if (const TWeakObjectPtr<AMO56PlayerController>* ControllerPtr = PlayerControllers.Find(PlayerId))
        {
                if (AMO56PlayerController* Controller = ControllerPtr->Get())
                {
                        if (APlayerState* PlayerState = Controller->GetPlayerState<APlayerState>())
                        {
                                PlayerData.PlayerName = PlayerState->GetPlayerName();
                                PlayerData.ControllerId = PlayerState->GetPlayerId();
                        }
                }
        }

        CurrentSaveGame->PlayerStates.Add(PlayerId, PlayerData);
}

void UMO56SaveSubsystem::ApplyCharacterStateFromSave(const FGuid& CharacterId)
{
        if (!CurrentSaveGame || !CharacterId.IsValid() || !IsAuthoritative())
        {
                return;
        }

        if (FCharacterSaveData* CharacterData = CurrentSaveGame->CharacterStates.Find(CharacterId))
        {
                if (CharacterData->InventoryId.IsValid())
                {
                        if (const TWeakObjectPtr<UInventoryComponent>* InventoryPtr = CharacterToInventoryComponent.Find(CharacterId))
                        {
                                if (UInventoryComponent* Inventory = InventoryPtr->Get())
                                {
                                        const FGuid CurrentId = Inventory->GetPersistentId();
                                        if (CurrentId != CharacterData->InventoryId)
                                        {
                                                RegisteredInventories.Remove(CurrentId);
                                                Inventory->OverridePersistentId(CharacterData->InventoryId);
                                                RegisteredInventories.Add(CharacterData->InventoryId, Inventory);
                                        }

                                        if (const FInventorySaveData* SavedInventory = CurrentSaveGame->InventoryStates.Find(CharacterData->InventoryId))
                                        {
                                                TGuardValue<bool> ApplyingGuard(bIsApplyingSave, true);
                                                Inventory->ReadFromSaveData(*SavedInventory);
                                        }
                                }
                        }
                }

                if (const TWeakObjectPtr<USkillSystemComponent>* SkillPtr = CharacterToSkillComponent.Find(CharacterId))
                {
                        if (USkillSystemComponent* Skill = SkillPtr->Get())
                        {
                                TGuardValue<bool> ApplyingGuard(bIsApplyingSave, true);
                                Skill->ReadFromSaveData(CharacterData->SkillState);
                        }
                }

                if (const TWeakObjectPtr<AMO56Character>* CharacterPtr = RegisteredCharacters.Find(CharacterId))
                {
                        if (AMO56Character* Character = CharacterPtr->Get())
                        {
                                Character->SetActorTransform(CharacterData->Transform);
                        }
                }

                UE_LOG(LogMO56SaveSubsystem, Log, TEXT("ApplyToCharacter: CharId=%s InvId=%s SkillsApplied=%d"), *CharacterId.ToString(), CharacterData->InventoryId.IsValid() ? *CharacterData->InventoryId.ToString() : TEXT("None"), CharacterData->SkillState.KnownSkills.Num());
        }
}

void UMO56SaveSubsystem::RefreshCharacterSaveData(const FGuid& CharacterId)
{
        if (!CurrentSaveGame || !CharacterId.IsValid() || !IsAuthoritative())
        {
                return;
        }

        FCharacterSaveData& CharacterData = CurrentSaveGame->CharacterStates.FindOrAdd(CharacterId);
        CharacterData.CharacterId = CharacterId;

        if (const TWeakObjectPtr<UInventoryComponent>* InventoryPtr = CharacterToInventoryComponent.Find(CharacterId))
        {
                if (UInventoryComponent* Inventory = InventoryPtr->Get())
                {
                        const FGuid InventoryId = Inventory->GetPersistentId();
                        CharacterData.InventoryId = InventoryId;

                        FInventorySaveData& InventoryData = CurrentSaveGame->InventoryStates.FindOrAdd(InventoryId);
                        Inventory->WriteToSaveData(InventoryData);

                        if (APawn* PawnOwner = Cast<APawn>(Inventory->GetOwner()))
                        {
                                CharacterData.Transform = PawnOwner->GetActorTransform();
                        }
                }
        }

        if (const TWeakObjectPtr<AMO56Character>* CharacterPtr = RegisteredCharacters.Find(CharacterId))
        {
                if (AMO56Character* Character = CharacterPtr->Get())
                {
                        CharacterData.Transform = Character->GetActorTransform();
                }
        }

        if (const TWeakObjectPtr<USkillSystemComponent>* SkillPtr = CharacterToSkillComponent.Find(CharacterId))
        {
                if (USkillSystemComponent* Skill = SkillPtr->Get())
                {
                        Skill->WriteToSaveData(CharacterData.SkillState);
                }
        }
}

