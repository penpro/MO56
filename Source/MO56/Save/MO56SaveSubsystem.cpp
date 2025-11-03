// Implementation: Registers inventories and world pickups, serializing them into the active
// UMO56SaveGame. Call SaveGame/LoadGame through this subsystem so multiplayer hosts persist
// shared state across sessions.
#include "Save/MO56SaveSubsystem.h"

#include "Algo/RemoveIf.h"
#include "MO56PlayerController.h"
#include "Menu/MO56MenuGameMode.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "InventoryComponent.h"
#include "ItemPickup.h"
#include "ItemData.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "Templates/UnrealTemplate.h"
#include "Containers/StringConv.h"
#include "MO56Character.h"
#include "Skills/SkillSystemComponent.h"
#include "HAL/FileManager.h"
#include "TimerManager.h"
#include "Save/MO56MenuSettingsSave.h"
#include "MO56VersionChecks.h"


DEFINE_LOG_CATEGORY_STATIC(LogMO56SaveSubsystem, Log, All);

namespace
{
static const TCHAR* const GameplayGameModeOption = TEXT("?game=/Game/MyStuff/BP_MO56GameMode.BP_MO56GameMode_C");
}

static FGuid GuidFromString(const FString& S)
{
        uint8 Digest[16];
        FTCHARToUTF8 Converted(*S);
        FMD5 Md5;
        Md5.Update(reinterpret_cast<const uint8*>(Converted.Get()), Converted.Length());
        Md5.Final(Digest);
        return FGuid(
                (Digest[0] << 24) | (Digest[1] << 16) | (Digest[2] << 8) | Digest[3],
                (Digest[4] << 24) | (Digest[5] << 16) | (Digest[6] << 8) | Digest[7],
                (Digest[8] << 24) | (Digest[9] << 16) | (Digest[10] << 8) | Digest[11],
                (Digest[12] << 24) | (Digest[13] << 16) | (Digest[14] << 8) | Digest[15]);
}

static FGuid StableGuidForActor(const AActor& Actor)
{
        const UWorld* W = Actor.GetWorld();
        const FString Map = W ? UWorld::RemovePIEPrefix(W->GetMapName()) : TEXT("UnknownMap");
        const FString Key = Actor.GetPathName() + TEXT("|") + Map;
        return GuidFromString(Key);
}

UMO56SaveSubsystem::UMO56SaveSubsystem()
{
        ActiveSaveSlotName = SaveSlotName;
        ActiveSaveUserIndex = SaveUserIndex;

        NonGameplayMapPrefixes = { TEXT("L_"), TEXT("Menu"), TEXT("MainMenu"), TEXT("Loading"), TEXT("Loading_"), TEXT("UI_") };
}

void UMO56SaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
        Super::Initialize(Collection);

        if (NonGameplayMapPrefixes.Num() == 0)
        {
                NonGameplayMapPrefixes = { TEXT("L_"), TEXT("Menu"), TEXT("MainMenu"), TEXT("UI_") };
        }

        if (PostLoadMapHandle.IsValid())
        {
                FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
                PostLoadMapHandle.Reset();
        }

        PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
                this, &UMO56SaveSubsystem::HandlePostLoadMapWithWorld);

        WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(
                this, &UMO56SaveSubsystem::HandleWorldCleanup);

        FWorldDelegates::OnPostWorldInitialization.AddUObject(
                this, &UMO56SaveSubsystem::HandlePostWorldInit);

        LoadOrCreateSaveGame();

        UpdateOrRebuildSaveIndex(true);
}

void UMO56SaveSubsystem::Deinitialize()
{
        Super::Deinitialize();

        FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);

        if (WorldCleanupHandle.IsValid())
        {
                FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
                WorldCleanupHandle.Reset();
        }

        if (PostLoadMapHandle.IsValid())
        {
                FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
                PostLoadMapHandle.Reset();
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
        bPendingApplyOnNextLevel = false;
        bPendingCreateNewSaveAfterTravel = false;
        PendingLoadedSave = nullptr;
}

TArray<FSaveIndexEntry> UMO56SaveSubsystem::ListSaves(bool bRebuildFromDiskIfMissing)
{
        if (!CachedSaveIndex)
        {
                CachedSaveIndex = Cast<UMO56SaveIndex>(UGameplayStatics::LoadGameFromSlot(SaveIndexSlotName, ActiveSaveUserIndex));
        }

        if (!CachedSaveIndex || bRebuildFromDiskIfMissing)
        {
                const bool bForceRebuild = (!CachedSaveIndex) || bRebuildFromDiskIfMissing;
                UpdateOrRebuildSaveIndex(bForceRebuild);
        }

        if (!CachedSaveIndex)
        {
                CachedSaveIndex = NewObject<UMO56SaveIndex>(this);
        }

        TArray<FSaveIndexEntry> Result = CachedSaveIndex->Entries;
        Result.RemoveAll([](const FSaveIndexEntry& Entry)
        {
                return Entry.SlotName.Equals(UMO56MenuSettingsSave::StaticSlotName);
        });
        Result.Sort([](const FSaveIndexEntry& A, const FSaveIndexEntry& B)
        {
                return A.UpdatedUtc > B.UpdatedUtc;
        });

        return Result;
}

bool UMO56SaveSubsystem::DoesSaveExist(const FGuid& SaveId) const
{
        if (!SaveId.IsValid())
        {
                return false;
        }

        if (CachedSaveIndex)
        {
                for (const FSaveIndexEntry& Entry : CachedSaveIndex->Entries)
                {
                        if (Entry.SaveId == SaveId)
                        {
                                return true;
                        }
                }
        }

        const FString SlotName = MakeSlotName(SaveId);
        if (SlotName.IsEmpty())
        {
                return false;
        }

        return UGameplayStatics::DoesSaveGameExist(SlotName, ActiveSaveUserIndex);
}

UMO56SaveGame* UMO56SaveSubsystem::PeekSaveHeader(const FGuid& SaveId)
{
        return ReadSave(SaveId, false);
}

void UMO56SaveSubsystem::StartNewGame(const FString& LevelName)
{
        UE_LOG(LogMO56SaveSubsystem, Display, TEXT("StartNewGame: %s"), *LevelName);

        bPendingCreateNewSaveAfterTravel = true;
        bPendingApplyOnNextLevel = false;
        PendingLoadedSave = nullptr;
        bAppliedPendingSaveThisLevel = false;

        FString TravelLevelName = LevelName;
        if (!TravelLevelName.IsEmpty())
        {
                TravelLevelName = FPackageName::GetShortName(TravelLevelName);
        }

        PendingLevelName = TravelLevelName.IsEmpty() ? LevelName : TravelLevelName;

        if (PendingLevelName.IsEmpty())
        {
                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("StartNewGame failed: invalid level name."));
                bPendingCreateNewSaveAfterTravel = false;
                return;
        }

        UE_LOG(LogMO56SaveSubsystem, Display, TEXT("StartNewGame travel target: %s"), *PendingLevelName);

        ActiveSaveId.Invalidate();
        CurrentSaveGame = nullptr;

        if (UWorld* World = GetWorld())
        {
                if (APlayerController* PC = World->GetFirstPlayerController())
                {
                        PC->SetInputMode(FInputModeGameOnly());
                        PC->bShowMouseCursor = false;
                }

                UE_LOG(LogMO56SaveSubsystem, Display, TEXT("StartNewGame OpenLevel: %s%s"), *PendingLevelName, GameplayGameModeOption);
                UGameplayStatics::OpenLevel(World, FName(*PendingLevelName), true, GameplayGameModeOption);
        }
        else
        {
                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("StartNewGame failed: no world."));
                bPendingCreateNewSaveAfterTravel = false;
        }
}

void UMO56SaveSubsystem::LoadSave(const FGuid& SaveId)
{
        UE_LOG(LogMO56SaveSubsystem, Display, TEXT("LoadSave: SaveId=%s"), *SaveId.ToString());

        if (!SaveId.IsValid())
        {
                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("LoadSave failed: invalid SaveId."));
                return;
        }

        if (UMO56SaveGame* Loaded = ReadSave(SaveId))
        {
                FString MapName = UWorld::RemovePIEPrefix(Loaded->LevelName);
                FString MapShortName = FPackageName::GetShortName(MapName);
                if (MapShortName.IsEmpty())
                {
                        MapShortName = TEXT("M_TestLevel");
                }

                PendingLoadedSave = Loaded;
                ActiveSaveId = SaveId;
                PendingLevelName = MapShortName;
                bPendingApplyOnNextLevel = true;
                bPendingCreateNewSaveAfterTravel = false;
                bAppliedPendingSaveThisLevel = false;
                CurrentSaveGame = nullptr;

                if (UWorld* World = GetWorld())
                {
                        const FString URL = MapShortName + GameplayGameModeOption;
                        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("LoadSave travel URL: %s"), *URL);

                        if (APlayerController* PC = World->GetFirstPlayerController())
                        {
                                PC->SetInputMode(FInputModeGameOnly());
                                PC->bShowMouseCursor = false;
                        }

                        UGameplayStatics::OpenLevel(World, FName(*MapShortName), true, GameplayGameModeOption);
                }
                else
                {
                        UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("LoadSave failed: no valid world context for travel."));
                        bPendingApplyOnNextLevel = false;
                        PendingLoadedSave = nullptr;
                }
        }
        else
        {
                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("LoadSave failed: SaveId=%s not found."), *SaveId.ToString());
                PendingLoadedSave = nullptr;
                bPendingApplyOnNextLevel = false;
        }
}

bool UMO56SaveSubsystem::SaveCurrentGame()
{
        if (!IsAuthoritative())
        {
                UE_LOG(LogMO56SaveSubsystem, Verbose, TEXT("SaveCurrentGame skipped on non-authority instance."));
                return false;
        }

        if (!CurrentSaveGame)
        {
                CreateNewSaveSlot();
        }

        if (!CurrentSaveGame)
        {
                return false;
        }

        const bool bResult = SaveGame(true);

        if (bResult)
        {
                UpdateOrRebuildSaveIndex();
        }

        return bResult;
}

bool UMO56SaveSubsystem::DeleteSave(const FGuid& SaveId)
{
        if (!SaveId.IsValid())
        {
                return false;
        }

        const FString SlotName = MakeSlotName(SaveId);
        const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, ActiveSaveUserIndex);

        if (bDeleted && CachedSaveIndex)
        {
                CachedSaveIndex->Entries.RemoveAll([&SaveId](const FSaveIndexEntry& Entry)
                {
                        return Entry.SaveId == SaveId;
                });
                UGameplayStatics::SaveGameToSlot(CachedSaveIndex, SaveIndexSlotName, ActiveSaveUserIndex);
        }

        return bDeleted;
}

bool UMO56SaveSubsystem::DeleteAllSaves(bool bAlsoDeleteIndex, bool bAlsoDeleteMenuSettings)
{
        UWorld* World = GetWorld();
        if (!World)
        {
                return false;
        }

        if (!IsMenuOrNonGameplayMap(World))
        {
                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("DeleteAllSaves blocked in gameplay world %s"), *World->GetMapName());
                return false;
        }

        const FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
        IFileManager& FM = IFileManager::Get();

        TArray<FString> Files;
        FM.FindFiles(Files, *(SaveDir / TEXT("*.sav")), true, false);

        int32 Deleted = 0;
        for (const FString& File : Files)
        {
                const FString Full = SaveDir / File;

                if (!bAlsoDeleteMenuSettings && File.Equals(TEXT("MO56_MenuSettings.sav"), ESearchCase::IgnoreCase))
                {
                        continue;
                }

                if (!bAlsoDeleteIndex && File.Equals(TEXT("MO56_SaveIndex.sav"), ESearchCase::IgnoreCase))
                {
                        continue;
                }

                if (FM.Delete(*Full, false, true, true))
                {
                        ++Deleted;
                        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("Deleted save file: %s"), *Full);
                }
                else
                {
                        UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("Failed to delete: %s"), *Full);
                }
        }

        ActiveSaveId.Invalidate();
        CurrentSaveGame = nullptr;
        PendingLoadedSave = nullptr;
        bPendingApplyOnNextLevel = false;

        UpdateOrRebuildSaveIndex(true);

        UE_LOG(LogMO56SaveSubsystem, Display, TEXT("DeleteAllSaves complete. Files deleted: %d"), Deleted);
        return true;
}

bool UMO56SaveSubsystem::SaveGame(bool bForce)
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
                if (!bForce && !CanAutosaveInWorld(*World))
                {
                        UE_LOG(LogMO56SaveSubsystem, Verbose, TEXT("SaveGame skipped for world %s."), *World->GetMapName());
                        return false;
                }

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
        if (CurrentSaveGame->CreatedUtc.GetTicks() == 0)
        {
                CurrentSaveGame->CreatedUtc = NowUtc;
        }

        CurrentSaveGame->UpdatedUtc = NowUtc;

        if (UWorld* World = GetWorld())
        {
                CurrentSaveGame->TotalPlayTimeSeconds = World->GetTimeSeconds();
                const FName LevelName = ResolveLevelName(*World);
                if (!LevelName.IsNone())
                {
                        CurrentSaveGame->LevelName = LevelName.ToString();
                }
        }

        CacheSaveMetadata(*CurrentSaveGame);

        const bool bSaved = WriteSave(CurrentSaveGame);
        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("SaveGame: Slot=%s Result=%s%s"),
                *CurrentSaveGame->SlotName,
                bSaved ? TEXT("Success") : TEXT("Failure"),
                bForce ? TEXT(" (forced)") : TEXT(""));

        if (bSaved && CachedSaveIndex)
        {
                UGameplayStatics::SaveGameToSlot(CachedSaveIndex, SaveIndexSlotName, ActiveSaveUserIndex);
        }

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
        bAppliedPendingSaveThisLevel = false;
        const FDateTime NowUtc = FDateTime::UtcNow();
        CurrentSaveGame->CreatedUtc = NowUtc;
        CurrentSaveGame->UpdatedUtc = NowUtc;
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

        ActiveSaveId = FGuid::NewGuid();
        CurrentSaveGame->SaveId = ActiveSaveId;
        CacheSaveMetadata(*CurrentSaveGame);

        if (CurrentSaveGame->LevelName.IsEmpty())
        {
                if (UWorld* World = GetWorld())
                {
                        const FName LevelName = ResolveLevelName(*World);
                        if (!LevelName.IsNone())
                        {
                                CurrentSaveGame->LevelName = LevelName.ToString();
                        }
                }
        }

        const bool bSaved = SaveGame();
        if (!bSaved)
        {
                return Summary;
        }

        UpdateOrRebuildSaveIndex();

        Summary.SlotName = CurrentSaveGame->SlotName;
        Summary.UserIndex = ActiveSaveUserIndex;
        Summary.InitialSaveTimestamp = CurrentSaveGame->CreatedUtc;
        Summary.LastSaveTimestamp = CurrentSaveGame->UpdatedUtc;
        Summary.TotalPlayTimeSeconds = CurrentSaveGame->TotalPlayTimeSeconds;
        Summary.LastLevelName = FName(*CurrentSaveGame->LevelName);
        Summary.InventoryCount = CurrentSaveGame->InventoryStates.Num();
        Summary.SaveId = CurrentSaveGame->SaveId;

        ActiveSaveSlotName = CurrentSaveGame ? CurrentSaveGame->SlotName : ActiveSaveSlotName;

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

        if (!CurrentSaveGame)
        {
                LoadOrCreateSaveGame();
        }

        FGuid CharacterId = Character->GetCharacterId();
        const FGuid InventoryId = Character->GetInventoryComponent() ? Character->GetInventoryComponent()->GetPersistentId() : FGuid();

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("RegisterPlayerCharacter: PlayerId=%s Controller=%s Pawn=%s CharId=%s InvId=%s"),
                *PlayerId.ToString(),
                *GetNameSafe(Controller),
                *GetNameSafe(Character),
                CharacterId.IsValid() ? *CharacterId.ToString() : TEXT("None"),
                InventoryId.IsValid() ? *InventoryId.ToString() : TEXT("None"));

        FGuid ResolvedCharacterId = CharacterId;
        FCharacterSaveData* CharacterData = nullptr;

        if (CurrentSaveGame)
        {
                for (auto& Pair : CurrentSaveGame->CharacterStates)
                {
                        if (Pair.Value.OwningPlayerId == PlayerId && Pair.Key.IsValid())
                        {
                                ResolvedCharacterId = Pair.Key;
                                CharacterData = &Pair.Value;
                                break;
                        }
                }

                if (!CharacterData && CharacterId.IsValid())
                {
                        CharacterData = CurrentSaveGame->CharacterStates.Find(CharacterId);
                        if (CharacterData)
                        {
                                ResolvedCharacterId = CharacterId;
                        }
                }

                if (const FGuid* LegacyInventoryId = CurrentSaveGame->PlayerInventoryIds.Find(PlayerId))
                {
                        const FGuid BindingId = ResolvedCharacterId.IsValid() ? ResolvedCharacterId : CharacterId;
                        FCharacterSaveData& CharacterDataRef = CurrentSaveGame->CharacterStates.FindOrAdd(BindingId);
                        CharacterDataRef.CharacterId = BindingId;
                        CharacterDataRef.InventoryId = *LegacyInventoryId;
                        CharacterDataRef.OwningPlayerId = PlayerId;

                        if (const FPlayerSaveData* PlayerData = CurrentSaveGame->PlayerStates.Find(PlayerId))
                        {
                                CharacterDataRef.SkillState = PlayerData->SkillState;
                                CharacterDataRef.Transform = PlayerData->Transform;
                        }

                        CurrentSaveGame->PlayerInventoryIds.Remove(PlayerId);

                        ResolvedCharacterId = BindingId;
                        CharacterData = &CharacterDataRef;
                }

                if (CharacterData)
                {
                        CharacterData->CharacterId = ResolvedCharacterId;
                        CharacterData->OwningPlayerId = PlayerId;
                }

                if (!CharacterData && ResolvedCharacterId.IsValid())
                {
                        FCharacterSaveData& NewData = CurrentSaveGame->CharacterStates.FindOrAdd(ResolvedCharacterId);
                        NewData.CharacterId = ResolvedCharacterId;
                        NewData.OwningPlayerId = PlayerId;
                        CharacterData = &NewData;
                }
        }

        if (ResolvedCharacterId.IsValid() && CharacterId.IsValid() && ResolvedCharacterId != CharacterId)
        {
                if (CurrentSaveGame)
                {
                        CurrentSaveGame->CharacterStates.Remove(CharacterId);
                }

                const FGuid OldCharacterId = CharacterId;
                Character->CharacterId = ResolvedCharacterId;
                CharacterId = ResolvedCharacterId;

                RegisteredCharacters.Remove(OldCharacterId);
                RegisteredCharacters.FindOrAdd(CharacterId) = Character;

                if (UInventoryComponent* CharacterInventory = Character->GetInventoryComponent())
                {
                        CharacterToInventoryComponent.Remove(OldCharacterId);
                        CharacterToInventoryComponent.FindOrAdd(CharacterId) = CharacterInventory;
                        InventoryOwnerIds.FindOrAdd(CharacterInventory) = CharacterId;
                }

                if (USkillSystemComponent* Skill = Character->GetSkillSystemComponent())
                {
                        CharacterToSkillComponent.Remove(OldCharacterId);
                        CharacterToSkillComponent.FindOrAdd(CharacterId) = Skill;
                        SkillComponentToCharacterId.FindOrAdd(Skill) = CharacterId;
                }
        }

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("RegisterPlayerCharacter: PlayerId=%s ResolvedCharId=%s"),
                *PlayerId.ToString(),
                CharacterId.IsValid() ? *CharacterId.ToString() : TEXT("None"));

        ApplyCharacterStateFromSave(CharacterId);
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
                const FGuid NewId = Pickup->WasSpawnedFromInventory()
                        ? FGuid::NewGuid()
                        : StableGuidForActor(*Pickup);
                Pickup->SetPersistentId(NewId);
        }

        ensureMsgf(Pickup->GetPersistentId().IsValid(), TEXT("Pickup persistent id must be valid after registration"));

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

        if (IsMenuOrNonGameplayMap(World))
        {
                UE_LOG(LogMO56SaveSubsystem, Verbose, TEXT("HandlePostWorldInit: skipping non-gameplay world %s"), *World->GetMapName());
                return;
        }

        bAppliedPendingSaveThisLevel = false;

        FDelegateHandle SpawnHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UMO56SaveSubsystem::HandleActorSpawned));
        WorldSpawnHandles.Add(World, SpawnHandle);

        for (TActorIterator<AItemPickup> It(World); It; ++It)
        {
                RegisterWorldPickup(*It);
        }

        const bool bAppliedPendingSave = ApplyPendingSave(*World);
        if (bAppliedPendingSave)
        {
                ApplySaveToWorld(World);
        }

        UE_LOG(LogMO56SaveSubsystem, Verbose, TEXT("HandlePostWorldInit: World=%s ready (gameplay)."), *UWorld::RemovePIEPrefix(World->GetMapName()));
}

void UMO56SaveSubsystem::HandlePostLoadMapWithWorld(UWorld* World)
{
        if (!World)
        {
                return;
        }

        const EWorldType::Type WorldType = World->WorldType;
        if (WorldType != EWorldType::Game && WorldType != EWorldType::PIE)
        {
                UE_LOG(LogMO56SaveSubsystem, Verbose, TEXT("HandlePostLoadMapWithWorld: skipping world %s (type=%d)"),
                        *UWorld::RemovePIEPrefix(World->GetMapName()), static_cast<int32>(WorldType));
                return;
        }

        const FString MapName = UWorld::RemovePIEPrefix(World->GetMapName());
        const bool bIsGameplayWorld = !IsMenuOrNonGameplayMap(World);
        FString MapShortName = FPackageName::GetShortName(MapName);
        if (MapShortName.IsEmpty())
        {
                MapShortName = MapName;
        }

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("HandlePostLoadMapWithWorld: %s (Gameplay=%s)"), *MapName, bIsGameplayWorld ? TEXT("true") : TEXT("false"));

        if (!bIsGameplayWorld)
        {
                return;
        }

        PendingLevelName = MapShortName;

        World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
                this, &UMO56SaveSubsystem::HandleDeferredBeginPlay, TWeakObjectPtr<UWorld>(World)));
}

void UMO56SaveSubsystem::HandleDeferredBeginPlay(TWeakObjectPtr<UWorld> WorldPtr)
{
        if (!WorldPtr.IsValid())
        {
                return;
        }

        UWorld* World = WorldPtr.Get();
        if (!World)
        {
                return;
        }

        const FString MapName = UWorld::RemovePIEPrefix(World->GetMapName());
        FString MapShortName = FPackageName::GetShortName(MapName);
        if (MapShortName.IsEmpty())
        {
                MapShortName = MapName;
        }
        const bool bIsGameplayWorld = !IsMenuOrNonGameplayMap(World);

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("BeginPlay: World=%s Gameplay=%s GM=%s PC=%s"),
                *MapShortName,
                bIsGameplayWorld ? TEXT("true") : TEXT("false"),
                *GetNameSafe(World->GetAuthGameMode()),
                *GetNameSafe(World->GetFirstPlayerController()));

        if (!bIsGameplayWorld)
        {
                return;
        }

        if (!World->HasBegunPlay() || !World->GetAuthGameMode())
        {
                UE_LOG(LogMO56SaveSubsystem, Verbose, TEXT("BeginPlay: World %s not ready (HasBegunPlay=%s GM=%s), retrying next tick."),
                        *MapShortName,
                        World->HasBegunPlay() ? TEXT("true") : TEXT("false"),
                        *GetNameSafe(World->GetAuthGameMode()));

                World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
                        this, &UMO56SaveSubsystem::HandleDeferredBeginPlay, WorldPtr));
                return;
        }

        PendingLevelName = MapShortName;

        bool bAppliedLoadedSave = bAppliedPendingSaveThisLevel;

        if (bPendingCreateNewSaveAfterTravel || !CurrentSaveGame)
        {
                bPendingCreateNewSaveAfterTravel = false;

                CreateNewSaveSlot();
                if (CurrentSaveGame)
                {
                        CurrentSaveGame->LevelName = MapShortName;
                        CurrentSaveGame->bIsGameplaySave = true;
                        CacheSaveMetadata(*CurrentSaveGame);

                        const bool bSaved = SaveGame(true);
                        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("Bootstrap save write: %s"), bSaved ? TEXT("ok") : TEXT("failed"));
                }
        }

        if (!bAppliedLoadedSave && bPendingApplyOnNextLevel && PendingLoadedSave)
        {
                const bool bAppliedNow = ApplyPendingSave(*World);
                bAppliedLoadedSave = bAppliedNow;
                if (bAppliedNow)
                {
                        ApplySaveToWorld(World);
                }
                else
                {
                        UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("Failed to apply loaded save on level %s."), *MapShortName);
                        bPendingApplyOnNextLevel = false;
                        PendingLoadedSave = nullptr;
                }
        }

        if (bAppliedLoadedSave)
        {
                bAppliedPendingSaveThisLevel = true;

                if (CurrentSaveGame)
                {
                        CurrentSaveGame->LevelName = MapShortName;
                        CurrentSaveGame->bIsGameplaySave = true;
                        CacheSaveMetadata(*CurrentSaveGame);

                        const bool bSaved = SaveGame(true);
                        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("Loaded run metadata refreshed (result=%s)."), bSaved ? TEXT("Success") : TEXT("Failure"));
                }

                UpdateOrRebuildSaveIndex(true);

                if (APlayerController* PC = World->GetFirstPlayerController())
                {
                        PC->SetInputMode(FInputModeGameOnly());
                        PC->bShowMouseCursor = false;
                }

                UE_LOG(LogMO56SaveSubsystem, Log, TEXT("Applied save on world %s"), *MapShortName);
        }
        else if (CurrentSaveGame)
        {
                CurrentSaveGame->LevelName = MapShortName;
                CurrentSaveGame->bIsGameplaySave = true;
                CacheSaveMetadata(*CurrentSaveGame);

                ApplySaveToWorld(World);
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
                        CacheSaveMetadata(*CurrentSaveGame);
                }
        }

        if (!CurrentSaveGame)
        {
                CurrentSaveGame = NewObject<UMO56SaveGame>(this);
                const FDateTime NowUtc = FDateTime::UtcNow();
                CurrentSaveGame->CreatedUtc = NowUtc;
                CurrentSaveGame->UpdatedUtc = NowUtc;
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
        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("ApplySaveToInventories: Begin Registered=%d"), RegisteredInventories.Num());

        int32 AppliedCount = 0;

        for (auto It = RegisteredInventories.CreateIterator(); It; ++It)
        {
                const FGuid InventoryId = It.Key();
                if (UInventoryComponent* Inventory = It.Value().Get())
                {
                        const EMO56InventoryOwner* OwnerTypePtr = InventoryOwnerTypes.Find(Inventory);
                        const bool bIsCharacterInventory = OwnerTypePtr && *OwnerTypePtr == EMO56InventoryOwner::Character;

                        if (bIsCharacterInventory)
                        {
                                continue;
                        }

                        if (const FInventorySaveData* Data = CurrentSaveGame->InventoryStates.Find(InventoryId))
                        {
                                Inventory->ReadFromSaveData(*Data);
                                ++AppliedCount;
                        }
                        else
                        {
                                FInventorySaveData EmptyData;
                                Inventory->ReadFromSaveData(EmptyData);
                                ++AppliedCount;
                        }
                }
                else
                {
                        It.RemoveCurrent();
                }
        }

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("ApplySaveToInventories: Completed Applied=%d"), AppliedCount);
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

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("ApplySaveToWorld: Begin Level=%s DroppedItems=%d Removed=%d"),
                *LevelName.ToString(),
                LevelState->DroppedItems.Num(),
                LevelState->RemovedPickupIds.Num());

        TSet<FGuid> PendingIds;
        for (const FWorldItemSaveData& Entry : LevelState->DroppedItems)
        {
                PendingIds.Add(Entry.PickupId);
        }

        int32 DestroyedCount = 0;
        int32 UpdatedCount = 0;
        int32 SpawnedCount = 0;

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
                        ++DestroyedCount;
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
                        ++UpdatedCount;
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
                ++SpawnedCount;
        }

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("ApplySaveToWorld: Completed Level=%s Updated=%d Spawned=%d Destroyed=%d"),
                *LevelName.ToString(),
                UpdatedCount,
                SpawnedCount,
                DestroyedCount);
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
        CacheSaveMetadata(*LoadedSave);
        CurrentSaveGame = LoadedSave;

        ApplySaveToInventories();

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("LoadGame: success"));
        return true;
}

bool UMO56SaveSubsystem::ApplyPendingSave(UWorld& World)
{
        if (!IsAuthoritative())
        {
                return false;
        }

        if (!bPendingApplyOnNextLevel || !PendingLoadedSave)
        {
                return false;
        }

        const FString MapName = UWorld::RemovePIEPrefix(World.GetMapName());
        FString MapShortName = FPackageName::GetShortName(MapName);
        if (MapShortName.IsEmpty())
        {
                MapShortName = MapName;
        }

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("ApplyPendingSave: Begin World=%s SaveId=%s"),
                *MapShortName,
                PendingLoadedSave->SaveId.IsValid() ? *PendingLoadedSave->SaveId.ToString() : TEXT("None"));

        PendingLoadedSave->LevelName = MapShortName;
        PendingLoadedSave->bIsGameplaySave = true;
        PendingLevelName = MapShortName;

        UMO56SaveGame* LoadedSave = PendingLoadedSave;
        PendingLoadedSave = nullptr;
        bPendingApplyOnNextLevel = false;

        const bool bApplied = ApplyLoadedSaveGame(LoadedSave);
        bAppliedPendingSaveThisLevel = bApplied;

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("ApplyPendingSave: Completed World=%s Result=%s"),
                *MapShortName,
                bApplied ? TEXT("Success") : TEXT("Failure"));

        return bApplied;
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
                const bool bApplied = ApplyLoadedSaveGame(LoadedSave);
                if (bApplied)
                {
                        if (UWorld* World = GetWorld())
                        {
                                ApplySaveToWorld(World);
                        }
                }

                return bApplied;
        }

        UE_LOG(LogMO56SaveSubsystem, Error, TEXT("LoadGame: save slot %s contained an incompatible object"), *SlotName);
        return false;
}

TArray<FSaveGameSummary> UMO56SaveSubsystem::GetAvailableSaveSummaries() const
{
        TArray<FSaveGameSummary> Result;

        if (UMO56SaveSubsystem* MutableThis = const_cast<UMO56SaveSubsystem*>(this))
        {
                const TArray<FSaveIndexEntry> Entries = MutableThis->ListSaves();
                for (const FSaveIndexEntry& Entry : Entries)
                {
                        FSaveGameSummary Summary;
                        Summary.SlotName = Entry.SlotName;
                        Summary.UserIndex = ActiveSaveUserIndex;
                        Summary.LastLevelName = Entry.LevelName.IsEmpty() ? NAME_None : FName(*Entry.LevelName);
                        Summary.LastSaveTimestamp = Entry.UpdatedUtc;
                        Summary.InitialSaveTimestamp = Entry.UpdatedUtc;
                        Summary.TotalPlayTimeSeconds = Entry.TotalPlaySeconds;
                        Summary.InventoryCount = 0;
                        Summary.SaveId = Entry.SaveId;

                        if (UMO56SaveGame* Header = MutableThis->PeekSaveHeader(Entry.SaveId))
                        {
                                Summary.InitialSaveTimestamp = Header->CreatedUtc;
                                Summary.LastSaveTimestamp = Header->UpdatedUtc;
                                Summary.TotalPlayTimeSeconds = Header->TotalPlayTimeSeconds;
                                Summary.LastLevelName = Header->LevelName.IsEmpty() ? NAME_None : FName(*Header->LevelName);
                                Summary.InventoryCount = Header->InventoryStates.Num();
                        }

                        Result.Add(MoveTemp(Summary));
                }
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
                if (!CanAutosaveInWorld(*World))
                {
                        return;
                }

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
                                const TWeakObjectPtr<UInventoryComponent>* Existing = RegisteredInventories.Find(CharacterData.InventoryId);
                                const bool bIdCollidesWithDifferentComp = Existing && Existing->Get() && Existing->Get() != InventoryComponent;

                                if (bIdCollidesWithDifferentComp)
                                {
                                        const FGuid NewInvId = InventoryComponent->GetPersistentId();
                                        if (const FInventorySaveData* ExistingData = CurrentSaveGame->InventoryStates.Find(CharacterData.InventoryId))
                                        {
                                                CurrentSaveGame->InventoryStates.FindOrAdd(NewInvId) = *ExistingData;
                                        }
                                        CharacterData.InventoryId = NewInvId;
                                }
                                else
                                {
                                        RegisteredInventories.Remove(InventoryId);
                                        InventoryComponent->OverridePersistentId(CharacterData.InventoryId);
                                        InventoryId = InventoryComponent->GetPersistentId();
                                }
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
                        InventoryId = CharacterData.InventoryId;

                        UE_LOG(LogMO56SaveSubsystem, Verbose, TEXT("Inv map [register]: Character %s -> InvId %s"),
                                *OwnerId.ToString(), *CharacterData.InventoryId.ToString());

                        const TWeakObjectPtr<UInventoryComponent>* ExistingPtr = RegisteredInventories.Find(CharacterData.InventoryId);
                        ensureMsgf(!(ExistingPtr && ExistingPtr->Get() && ExistingPtr->Get() != InventoryComponent),
                                TEXT("Two live inventories share InventoryId %s"), *CharacterData.InventoryId.ToString());
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
                if (OwnerType != EMO56InventoryOwner::Character)
                {
                        TGuardValue<bool> ApplyingGuard(bIsApplyingSave, true);
                        InventoryComponent->ReadFromSaveData(*SavedData);
                }
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

        if (UWorld* World = GetWorld())
        {
                if (!CanAutosaveInWorld(*World))
                {
                        return;
                }
        }

        RefreshInventorySaveData();
        SaveGame();
}



FString UMO56SaveSubsystem::MakeSlotName(const FGuid& SaveId) const
{
        if (!SaveId.IsValid())
        {
                return FString();
        }

        return FString::Printf(TEXT("MO56_%s"), *SaveId.ToString(EGuidFormats::DigitsWithHyphens));
}

bool UMO56SaveSubsystem::IsMenuOrNonGameplayMap(const UWorld* World) const
{
        if (!World)
        {
                return true;
        }

        if (const AGameModeBase* GameMode = World->GetAuthGameMode())
        {
                if (GameMode->IsA(AMO56MenuGameMode::StaticClass()))
                {
                        return true;
                }
        }

        return IsMenuOrNonGameplayMapName(World->GetMapName());
}

bool UMO56SaveSubsystem::CanAutosaveInWorld(const UWorld& World) const
{
        if (IsMenuOrNonGameplayMap(&World))
        {
                return false;
        }

        const FName LevelName = ResolveLevelName(World);
        if (LevelName.IsNone())
        {
                        return false;
        }

        if (GameplayMapNames.Num() > 0 && !IsGameplayMapName(LevelName))
        {
                return false;
        }

        return true;
}

bool UMO56SaveSubsystem::IsGameplayMapName(const FName& MapName) const
{
        if (GameplayMapNames.Num() == 0)
        {
                return true;
        }

        if (GameplayMapNames.Contains(MapName))
        {
                return true;
        }

        const FString MapString = MapName.ToString();
        const FString Sanitized = UWorld::RemovePIEPrefix(MapString);
        return GameplayMapNames.Contains(FName(*Sanitized));
}

bool UMO56SaveSubsystem::IsMenuOrNonGameplayMapName(const FString& MapName) const
{
        if (MapName.IsEmpty())
        {
                return false;
        }

        const FString Sanitized = UWorld::RemovePIEPrefix(MapName);
        for (const FString& Prefix : NonGameplayMapPrefixes)
        {
                if (!Prefix.IsEmpty() && Sanitized.StartsWith(Prefix))
                {
                        return true;
                }
        }

        return false;
}

FString UMO56SaveSubsystem::GetSaveDir() const
{
        return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
}

bool UMO56SaveSubsystem::IsSaveFileName(const FString& Name) const
{
        return Name.StartsWith(TEXT("MO56_")) && Name.EndsWith(TEXT(".sav"));
}

bool UMO56SaveSubsystem::WriteSave(const UMO56SaveGame* Data)
{
        if (!Data)
        {
                return false;
        }

        return UGameplayStatics::SaveGameToSlot(const_cast<UMO56SaveGame*>(Data), Data->SlotName, ActiveSaveUserIndex);
}

UMO56SaveGame* UMO56SaveSubsystem::ReadSave(const FGuid& SaveId, bool bUpdateMetadata)
{
        if (!SaveId.IsValid())
        {
                return nullptr;
        }

        const FString SlotName = MakeSlotName(SaveId);
        if (UGameplayStatics::DoesSaveGameExist(SlotName, ActiveSaveUserIndex))
        {
                if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, ActiveSaveUserIndex))
                {
                        if (UMO56SaveGame* SaveGame = Cast<UMO56SaveGame>(Loaded))
                        {
                                if (bUpdateMetadata)
                                {
                                        CacheSaveMetadata(*SaveGame);
                                }
                                return SaveGame;
                        }
                        else
                        {
                                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("ReadSave: slot %s is not a UMO56SaveGame"), *SlotName);
                        }
                }
        }

        return nullptr;
}

void UMO56SaveSubsystem::UpdateOrRebuildSaveIndex(bool bForceRebuild)
{
        if (CachedSaveIndex && CachedSaveIndex->GetClass() != UMO56SaveIndex::StaticClass())
        {
                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("SaveIndex mismatch; rebuilding from disk"));
                CachedSaveIndex = nullptr;
                bForceRebuild = true;
        }

        if (!CachedSaveIndex)
        {
                if (USaveGame* LoadedIndex = UGameplayStatics::LoadGameFromSlot(SaveIndexSlotName, ActiveSaveUserIndex))
                {
                        if (UMO56SaveIndex* LoadedSaveIndex = Cast<UMO56SaveIndex>(LoadedIndex))
                        {
                                CachedSaveIndex = LoadedSaveIndex;
                        }
                        else
                        {
                                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("Save index slot contained unexpected class. Rebuilding index."));
                                UGameplayStatics::DeleteGameInSlot(SaveIndexSlotName, ActiveSaveUserIndex);
                                bForceRebuild = true;
                        }
                }
        }

        if (!CachedSaveIndex)
        {
                CachedSaveIndex = NewObject<UMO56SaveIndex>(this);
                bForceRebuild = true;
        }

        if (!bForceRebuild)
        {
                return;
        }

        const FString Directory = GetSaveDir();
        IFileManager::Get().MakeDirectory(*Directory, true);

        TArray<FString> SkippedSlots;
        int32 FilesFound = 0;
        int32 AddedEntries = 0;
        const FGuid PreviousActiveSaveId = ActiveSaveId;
        const FString PreviousActiveSlotName = ActiveSaveSlotName;

        CachedSaveIndex->Entries.Reset();

        IFileManager::Get().IterateDirectory(*Directory, [this, &SkippedSlots, &FilesFound, &AddedEntries](const TCHAR* Path, bool bIsDirectory)
        {
                if (bIsDirectory)
                {
                        return true;
                }

                const FString FilePath(Path);
                if (!FilePath.EndsWith(TEXT(".sav")))
                {
                        return true;
                }

                ++FilesFound;

                const FString FileName = FPaths::GetCleanFilename(FilePath);
                const FString SlotName = FPaths::GetBaseFilename(FileName);

                if (!IsSaveFileName(FileName) || SlotName.Equals(SaveIndexSlotName) || SlotName.Equals(UMO56MenuSettingsSave::StaticSlotName))
                {
                        if (!SlotName.IsEmpty())
                        {
                                SkippedSlots.AddUnique(SlotName);
                        }
                        return true;
                }

                if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, ActiveSaveUserIndex))
                {
                        if (UMO56SaveGame* SaveGame = Cast<UMO56SaveGame>(Loaded))
                        {
                                CacheSaveMetadata(*SaveGame);

                                if (!SaveGame->bIsGameplaySave || IsMenuOrNonGameplayMapName(SaveGame->LevelName))
                                {
                                        SkippedSlots.AddUnique(SlotName);
                                        return true;
                                }

                                FSaveIndexEntry Entry;
                                Entry.SaveId = SaveGame->SaveId;
                                Entry.SlotName = SaveGame->SlotName;
                                Entry.LevelName = SaveGame->LevelName;
                                Entry.UpdatedUtc = SaveGame->UpdatedUtc;
                                Entry.TotalPlaySeconds = SaveGame->TotalPlayTimeSeconds;

                                CachedSaveIndex->Entries.Add(Entry);
                                ++AddedEntries;
                        }
                        else if (!Loaded->IsA(UMO56MenuSettingsSave::StaticClass()))
                        {
                                SkippedSlots.AddUnique(SlotName);
                                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("UpdateOrRebuildSaveIndex: slot %s contained unexpected class."), *SlotName);
                        }
                }

                return true;
        });

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("Scanning save directory: %s"), *Directory);
        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("Found %d save files; indexed %d gameplay entries."), FilesFound, AddedEntries);

        if (SkippedSlots.Num() > 0)
        {
                UE_LOG(LogMO56SaveSubsystem, Log, TEXT("Skipped save slots: %s"), *FString::Join(SkippedSlots, TEXT(", ")));
        }

        UGameplayStatics::SaveGameToSlot(CachedSaveIndex, SaveIndexSlotName, ActiveSaveUserIndex);

        ActiveSaveId = PreviousActiveSaveId;
        ActiveSaveSlotName = PreviousActiveSlotName;
}

void UMO56SaveSubsystem::CacheSaveMetadata(UMO56SaveGame& SaveGame)
{
        if (!CachedSaveIndex)
        {
                CachedSaveIndex = NewObject<UMO56SaveIndex>(this);
        }

        if (!SaveGame.SaveId.IsValid())
        {
                SaveGame.SaveId = ActiveSaveId.IsValid() ? ActiveSaveId : FGuid::NewGuid();
        }

        ActiveSaveId = SaveGame.SaveId;

        SaveGame.SaveVersion = MO56_SAVE_VERSION;
        SaveGame.SlotName = MakeSlotName(SaveGame.SaveId);
        SaveGame.LevelName = UWorld::RemovePIEPrefix(SaveGame.LevelName);

        if (SaveGame.CreatedUtc.GetTicks() == 0)
        {
                SaveGame.CreatedUtc = FDateTime::UtcNow();
        }

        SaveGame.UpdatedUtc = FDateTime::UtcNow();

        ActiveSaveSlotName = SaveGame.SlotName;

        bool bGameplaySave = true;
        if (UWorld* World = GetWorld())
        {
                bGameplaySave = !IsMenuOrNonGameplayMap(World);
        }
        else if (!SaveGame.LevelName.IsEmpty())
        {
                bGameplaySave = !IsMenuOrNonGameplayMapName(SaveGame.LevelName);
        }

        SaveGame.bIsGameplaySave = bGameplaySave;

        if (CachedSaveIndex)
        {
                FSaveIndexEntry* ExistingEntry = CachedSaveIndex->Entries.FindByPredicate([&SaveGame](const FSaveIndexEntry& Entry)
                {
                        return Entry.SaveId == SaveGame.SaveId;
                });

                if (!ExistingEntry)
                {
                        FSaveIndexEntry NewEntry;
                        NewEntry.SaveId = SaveGame.SaveId;
                        NewEntry.SlotName = SaveGame.SlotName;
                        NewEntry.LevelName = SaveGame.LevelName;
                        NewEntry.UpdatedUtc = SaveGame.UpdatedUtc;
                        NewEntry.TotalPlaySeconds = SaveGame.TotalPlayTimeSeconds;
                        CachedSaveIndex->Entries.Add(NewEntry);
                }
                else
                {
                        ExistingEntry->SlotName = SaveGame.SlotName;
                        ExistingEntry->LevelName = SaveGame.LevelName;
                        ExistingEntry->UpdatedUtc = SaveGame.UpdatedUtc;
                        ExistingEntry->TotalPlaySeconds = SaveGame.TotalPlayTimeSeconds;
                }
        }
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
                                                const TWeakObjectPtr<UInventoryComponent>* Existing = RegisteredInventories.Find(CharacterData->InventoryId);
                                                const bool bIdCollidesWithDifferentComp = Existing && Existing->Get() && Existing->Get() != Inventory;

                                                if (bIdCollidesWithDifferentComp)
                                                {
                                                        const FGuid NewInvId = CurrentId;
                                                        if (const FInventorySaveData* ExistingData = CurrentSaveGame->InventoryStates.Find(CharacterData->InventoryId))
                                                        {
                                                                CurrentSaveGame->InventoryStates.FindOrAdd(NewInvId) = *ExistingData;
                                                        }
                                                        CharacterData->InventoryId = NewInvId;
                                                }
                                                else
                                                {
                                                        RegisteredInventories.Remove(CurrentId);
                                                        Inventory->OverridePersistentId(CharacterData->InventoryId);
                                                        RegisteredInventories.Add(CharacterData->InventoryId, Inventory);
                                                }
                                        }

                                        const FInventorySaveData* SavedInventory = CurrentSaveGame->InventoryStates.Find(CharacterData->InventoryId);
                                        if (SavedInventory)
                                        {
                                                TGuardValue<bool> ApplyingGuard(bIsApplyingSave, true);
                                                Inventory->ReadFromSaveData(*SavedInventory);
                                        }

                                        ensureMsgf(CharacterData->InventoryId.IsValid(), TEXT("Character %s has invalid InventoryId after apply"), *CharacterId.ToString());
                                        const TWeakObjectPtr<UInventoryComponent>* CurrentPtr = RegisteredInventories.Find(CurrentId);
                                        ensureMsgf(!CurrentPtr || CurrentPtr->Get() == Inventory,
                                                TEXT("RegisteredInventories map inconsistent for %s"), *CharacterId.ToString());

                                        const bool bHadSaved = CurrentSaveGame && CurrentSaveGame->InventoryStates.Contains(CharacterData->InventoryId);

                                        UE_LOG(LogMO56SaveSubsystem, Verbose,
                                            TEXT("Inv map: Character %s -> InvId %s (hadSaved=%s)"),
                                            *CharacterId.ToString(),
                                            *CharacterData->InventoryId.ToString(),
                                            bHadSaved ? TEXT("true") : TEXT("false"));

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

                UE_LOG(LogMO56SaveSubsystem, Log,
                        TEXT("ApplyCharacterStateFromSave: CharacterId=%s InventoryId=%s"),
                        *CharacterId.ToString(),
                        CharacterData->InventoryId.IsValid() ? *CharacterData->InventoryId.ToString() : TEXT("None"));
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

        FGuid OwningPlayerId;
        if (const TWeakObjectPtr<AMO56Character>* CharacterPtr = RegisteredCharacters.Find(CharacterId))
        {
                if (AMO56Character* Character = CharacterPtr->Get())
                {
                        for (const auto& ControllerPair : PlayerControllers)
                        {
                                if (AMO56PlayerController* PlayerController = ControllerPair.Value.Get())
                                {
                                        if (APawn* PossessedPawn = PlayerController->GetPawn())
                                        {
                                                if (PossessedPawn == Character)
                                                {
                                                        OwningPlayerId = ControllerPair.Key;
                                                        break;
                                                }

                                                if (!OwningPlayerId.IsValid())
                                                {
                                                        if (const AMO56Character* PossessedCharacter = Cast<AMO56Character>(PossessedPawn))
                                                        {
                                                                if (PossessedCharacter->GetCharacterId() == CharacterId)
                                                                {
                                                                        OwningPlayerId = ControllerPair.Key;
                                                                        break;
                                                                }
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }

        CharacterData.OwningPlayerId = OwningPlayerId;
}

