#include "Save/MO56SaveSubsystem.h"

#include "Algo/RemoveIf.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "InventoryComponent.h"
#include "ItemPickup.h"
#include "ItemData.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Templates/UnrealTemplate.h"
#include "MO56Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogMO56SaveSubsystem, Log, All);

UMO56SaveSubsystem::UMO56SaveSubsystem()
{
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
}

bool UMO56SaveSubsystem::SaveGame()
{
        if (!CurrentSaveGame)
        {
                LoadOrCreateSaveGame();
        }

        RefreshInventorySaveData();
        RefreshTrackedPickups();

        if (!CurrentSaveGame)
        {
                return false;
        }

        const bool bSaved = UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SaveSlotName, SaveUserIndex);
        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("SaveGame: %s"), bSaved ? TEXT("Success") : TEXT("Failure"));
        return bSaved;
}

bool UMO56SaveSubsystem::LoadGame()
{
        USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex);
        if (!Loaded)
        {
                UE_LOG(LogMO56SaveSubsystem, Warning, TEXT("LoadGame: no save present for slot %s"), SaveSlotName);
                return false;
        }

        if (UMO56SaveGame* LoadedSave = Cast<UMO56SaveGame>(Loaded))
        {
                CurrentSaveGame = LoadedSave;
                ApplySaveToInventories();

                if (UWorld* World = GetWorld())
                {
                        ApplySaveToWorld(World);
                }

                UE_LOG(LogMO56SaveSubsystem, Log, TEXT("LoadGame: success"));
                return true;
        }

        UE_LOG(LogMO56SaveSubsystem, Error, TEXT("LoadGame: save slot %s contained an incompatible object"), SaveSlotName);
        return false;
}

void UMO56SaveSubsystem::ResetToNewGame()
{
        CurrentSaveGame = NewObject<UMO56SaveGame>(this);

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

        UE_LOG(LogMO56SaveSubsystem, Log, TEXT("ResetToNewGame: save data cleared."));
}

void UMO56SaveSubsystem::RegisterInventoryComponent(UInventoryComponent* InventoryComponent, bool bIsPlayerInventory)
{
        if (!InventoryComponent)
        {
                return;
        }

        InventoryComponent->EnsurePersistentId();
        const FGuid InventoryId = InventoryComponent->GetPersistentId();
        RegisteredInventories.Add(InventoryId, InventoryComponent);

        if (!CurrentSaveGame)
        {
                LoadOrCreateSaveGame();
        }

        if (bIsPlayerInventory && CurrentSaveGame)
        {
                CurrentSaveGame->PlayerInventoryId = InventoryId;
        }

        if (CurrentSaveGame)
        {
                if (const FInventorySaveData* SavedData = CurrentSaveGame->InventoryStates.Find(InventoryId))
                {
                        InventoryComponent->ReadFromSaveData(*SavedData);
                }
                else if (bIsPlayerInventory && CurrentSaveGame->PlayerInventoryId.IsValid())
                {
                        if (const FInventorySaveData* PlayerData = CurrentSaveGame->InventoryStates.Find(CurrentSaveGame->PlayerInventoryId))
                        {
                                InventoryComponent->ReadFromSaveData(*PlayerData);
                        }
                }
        }
}

void UMO56SaveSubsystem::UnregisterInventoryComponent(UInventoryComponent* InventoryComponent)
{
        if (!InventoryComponent)
        {
                return;
        }

        const FGuid InventoryId = InventoryComponent->GetPersistentId();
        RegisteredInventories.Remove(InventoryId);
}

void UMO56SaveSubsystem::RegisterWorldPickup(AItemPickup* Pickup)
{
        if (!Pickup)
        {
                return;
        }

        if (!Pickup->GetPersistentId().IsValid())
        {
                Pickup->SetPersistentId(Pickup->GetActorGuid());
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

        FDelegateHandle SpawnHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UMO56SaveSubsystem::HandleActorSpawned));
        WorldSpawnHandles.Add(World, SpawnHandle);

        for (TActorIterator<AItemPickup> It(World); It; ++It)
        {
                RegisterWorldPickup(*It);
        }

        ApplySaveToWorld(World);
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

        if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex))
        {
                CurrentSaveGame = Cast<UMO56SaveGame>(Loaded);
        }

        if (!CurrentSaveGame)
        {
                CurrentSaveGame = NewObject<UMO56SaveGame>(this);
        }
}

void UMO56SaveSubsystem::ApplySaveToInventories()
{
        if (!CurrentSaveGame)
        {
                return;
        }

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

                if (IsPlayerInventoryComponent(Inventory))
                {
                        CurrentSaveGame->PlayerInventoryId = InventoryId;

                        if (APawn* OwningPawn = Cast<APawn>(Inventory->GetOwner()))
                        {
                                CurrentSaveGame->PlayerTransform = OwningPawn->GetActorTransform();
                        }
                }
        }
}

void UMO56SaveSubsystem::RefreshTrackedPickups()
{
        if (!CurrentSaveGame)
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

bool UMO56SaveSubsystem::IsPlayerInventoryComponent(const UInventoryComponent* InventoryComponent) const
{
        if (!InventoryComponent)
        {
                return false;
        }

        const AActor* Owner = InventoryComponent->GetOwner();
        return Owner && Owner->IsA<AMO56Character>();
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

