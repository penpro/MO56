// Implementation: Configure inventory slots in C++ or blueprints, then build UI widgets
// that call the provided getters/mutators. Attach this component to pawns or containers
// and ensure authority drives mutations so replication keeps clients in sync.
#include "InventoryComponent.h"
#include "ItemData.h"
#include "ItemPickup.h"

#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "UObject/Package.h"
#include "Net/UnrealNetwork.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    EnsureSlotCapacity();
    SetIsReplicatedByDefault(true);
}

void UInventoryComponent::PostInitProperties()
{
    Super::PostInitProperties();

    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        EnsurePersistentId();
    }
}

void UInventoryComponent::OnRep_Slots()
{
    OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::InitializeComponent()
{
    Super::InitializeComponent();

    EnsurePersistentId();
    EnsureSlotCapacity();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UInventoryComponent, Slots);
}

int32 FItemStack::MaxStack() const
{
    if (!Item) return 0;
    return FMath::Max(1, Item->MaxStackSize);
}

int32 UInventoryComponent::AddItem(UItemData* Item, int32 Count)
{
    if (!Item || Count <= 0) return 0;
    if (GetOwner() && GetOwner()->GetLocalRole() != ROLE_Authority)
    {
        UE_LOG(LogTemp, Warning, TEXT("AddItem should be called on the server for replicated inventories."));
    }
    EnsureSlotCapacity();
    int32 Remaining = Count;
    Remaining -= AddToExistingStacks(Item, Remaining);
    Remaining -= AddToEmptySlots(Item, Remaining);
    const int32 Added = Count - Remaining;
    if (Added > 0)
    {
        OnInventoryUpdated.Broadcast();
    }
    return Added;
}

int32 UInventoryComponent::AddToExistingStacks(UItemData* Item, int32 Count)
{
    int32 Added = 0;
    const int32 Max = FItemStack{ Item, 0 }.MaxStack();
    for (FItemStack& Slot : Slots)
    {
        if (Count <= 0) break;
        if (Slot.Item == Item && Slot.Quantity < Max)
        {
            const int32 Space = Max - Slot.Quantity;
            const int32 ToAdd = FMath::Min(Space, Count);
            Slot.Quantity += ToAdd;
            Added += ToAdd;
            Count -= ToAdd;
        }
    }
    return Added;
}

int32 UInventoryComponent::AddToEmptySlots(UItemData* Item, int32 Count)
{
    int32 Added = 0;
    EnsureSlotCapacity();
    const int32 Max = FItemStack{ Item, 0 }.MaxStack();
    for (FItemStack& Slot : Slots)
    {
        if (Count <= 0) break;
        if (Slot.IsEmpty())
        {
            Slot.Item = Item;
            const int32 ToAdd = FMath::Min(Max, Count);
            Slot.Quantity = ToAdd;
            Added += ToAdd;
            Count -= ToAdd;
        }
    }
    return Added;
}

int32 UInventoryComponent::RemoveItem(UItemData* Item, int32 Count)
{
    if (!Item || Count <= 0) return 0;
    if (GetOwner() && GetOwner()->GetLocalRole() != ROLE_Authority)
    {
        UE_LOG(LogTemp, Warning, TEXT("RemoveItem should be called on the server for replicated inventories."));
    }
    EnsureSlotCapacity();
    int32 Removed = 0;
    for (FItemStack& Slot : Slots)
    {
        if (Count <= 0) break;
        if (Slot.Item == Item)
        {
            const int32 ToRemove = FMath::Min(Slot.Quantity, Count);
            Slot.Quantity -= ToRemove;
            Removed += ToRemove;
            Count -= ToRemove;
            if (Slot.Quantity <= 0)
            {
                Slot.Quantity = 0;
                Slot.Item = nullptr;
            }
        }
    }
    if (Removed > 0)
    {
        OnInventoryUpdated.Broadcast();
    }
    return Removed;
}

int32 UInventoryComponent::CountItem(UItemData* Item) const
{
    if (!Item) return 0;
    int32 Total = 0;
    for (const FItemStack& Slot : Slots)
    {
        if (Slot.Item == Item) { Total += Slot.Quantity; }
    }
    return Total;
}

void UInventoryComponent::GetSlotAtIndex(int32 SlotIndex, FItemStack& OutSlot) const
{
    if (Slots.IsValidIndex(SlotIndex))
    {
        OutSlot = Slots[SlotIndex];
    }
    else
    {
        OutSlot = FItemStack{};
    }
}

bool UInventoryComponent::DebugSetSlot(int32 SlotIndex, UItemData* Item, int32 Quantity)
{
    if (!Slots.IsValidIndex(SlotIndex) || Quantity < 0)
    {
        return false;
    }

    if (GetOwner() && GetOwner()->GetLocalRole() != ROLE_Authority)
    {
        UE_LOG(LogTemp, Warning, TEXT("DebugSetSlot must be executed on the authority for replicated inventories."));
        return false;
    }

    FItemStack& Slot = Slots[SlotIndex];
    Slot.Item = Item;
    Slot.Quantity = Quantity;

    if (Slot.Quantity <= 0)
    {
        Slot.Quantity = 0;
        Slot.Item = nullptr;
    }

    OnInventoryUpdated.Broadcast();
    return true;
}

bool UInventoryComponent::SplitStackAtIndex(int32 SlotIndex)
{
    EnsureSlotCapacity();

    if (!Slots.IsValidIndex(SlotIndex))
    {
        return false;
    }

    FItemStack& Slot = Slots[SlotIndex];
    if (Slot.IsEmpty() || Slot.Quantity <= 1)
    {
        return false;
    }

    const int32 AmountToMove = FMath::Max(1, Slot.Quantity / 2);
    if (AmountToMove <= 0)
    {
        return false;
    }

    const int32 OriginalQuantity = Slot.Quantity;
    Slot.Quantity -= AmountToMove;

    const int32 Added = AddToEmptySlots(Slot.Item, AmountToMove);
    if (Added != AmountToMove)
    {
        Slot.Quantity = OriginalQuantity;
        return false;
    }

    OnInventoryUpdated.Broadcast();
    return true;
}

bool UInventoryComponent::DestroyItemAtIndex(int32 SlotIndex)
{
    EnsureSlotCapacity();

    if (!Slots.IsValidIndex(SlotIndex))
    {
        return false;
    }

    FItemStack& Slot = Slots[SlotIndex];
    if (Slot.IsEmpty())
    {
        return false;
    }

    Slot.Item = nullptr;
    Slot.Quantity = 0;

    OnInventoryUpdated.Broadcast();
    return true;
}

bool UInventoryComponent::DropItemAtIndex(int32 SlotIndex)
{
    if (ActiveDropAllTimers.Contains(SlotIndex))
    {
        return true;
    }

    if (!DropSingleItemInternal(SlotIndex))
    {
        return false;
    }

    EnsureSlotCapacity();

    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty())
    {
        return true;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return true;
    }

    constexpr float DropInterval = 0.2f;
    FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UInventoryComponent::HandleDropAllTimerTick, SlotIndex);
    FTimerHandle& TimerHandle = ActiveDropAllTimers.FindOrAdd(SlotIndex);
    World->GetTimerManager().ClearTimer(TimerHandle);
    World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, DropInterval, true);

    return true;
}

bool UInventoryComponent::DropSingleItemAtIndex(int32 SlotIndex)
{
    if (ActiveDropAllTimers.Contains(SlotIndex))
    {
        return false;
    }

    return DropSingleItemInternal(SlotIndex);
}

bool UInventoryComponent::TransferItemToInventory(UInventoryComponent* TargetInventory, int32 SourceSlotIndex, int32 TargetSlotIndex)
{
    if (!TargetInventory || TargetInventory == this)
    {
        return false;
    }

    EnsureSlotCapacity();
    TargetInventory->EnsureSlotCapacity();

    if (!Slots.IsValidIndex(SourceSlotIndex) || !TargetInventory->Slots.IsValidIndex(TargetSlotIndex))
    {
        return false;
    }

    FItemStack& SourceSlot = Slots[SourceSlotIndex];
    if (SourceSlot.IsEmpty())
    {
        return false;
    }

    FItemStack& TargetSlot = TargetInventory->Slots[TargetSlotIndex];

    const UItemData* SourceItem = SourceSlot.Item;
    if (!SourceItem)
    {
        return false;
    }

    bool bChangedSource = false;
    bool bChangedTarget = false;

    if (TargetSlot.IsEmpty())
    {
        const int32 MaxStack = SourceSlot.MaxStack();
        const int32 TransferAmount = FMath::Clamp(SourceSlot.Quantity, 0, MaxStack);
        if (TransferAmount <= 0)
        {
            return false;
        }

        TargetSlot.Item = SourceSlot.Item;
        TargetSlot.Quantity = TransferAmount;

        SourceSlot.Quantity -= TransferAmount;
        if (SourceSlot.Quantity <= 0)
        {
            SourceSlot.Item = nullptr;
            SourceSlot.Quantity = 0;
        }

        bChangedSource = true;
        bChangedTarget = true;
    }
    else if (TargetSlot.Item == SourceSlot.Item)
    {
        const int32 MaxStack = TargetSlot.MaxStack();
        const int32 AvailableSpace = FMath::Max(0, MaxStack - TargetSlot.Quantity);
        if (AvailableSpace <= 0)
        {
            return false;
        }

        const int32 TransferAmount = FMath::Min(AvailableSpace, SourceSlot.Quantity);
        if (TransferAmount <= 0)
        {
            return false;
        }

        TargetSlot.Quantity += TransferAmount;
        SourceSlot.Quantity -= TransferAmount;

        if (SourceSlot.Quantity <= 0)
        {
            SourceSlot.Item = nullptr;
            SourceSlot.Quantity = 0;
        }

        bChangedSource = true;
        bChangedTarget = true;
    }
    else
    {
        Swap(TargetSlot, SourceSlot);
        bChangedSource = true;
        bChangedTarget = true;
    }

    if (bChangedSource)
    {
        OnInventoryUpdated.Broadcast();
    }

    if (bChangedTarget)
    {
        TargetInventory->OnInventoryUpdated.Broadcast();
    }

    return true;
}

namespace
{
    constexpr float KgToLbs = 2.20462262f;
    constexpr float Cm3ToM3 = 1.0e-6f;

    float ResolveItemWeightKg(const UItemData* Item)
    {
        if (!Item)
        {
            return 0.f;
        }

        if (Item->WeightKgOverride > 0.f)
        {
            return Item->WeightKgOverride;
        }

        if (Item->WeightLbsOverride > 0.f)
        {
            return Item->WeightLbsOverride / KgToLbs;
        }

        if (UStaticMesh* StaticMesh = Item->WorldStaticMesh.LoadSynchronous())
        {
            if (UBodySetup* BodySetup = StaticMesh->GetBodySetup())
            {
                float Mass = BodySetup->CalculateMass();
                const float ScaleFactor = FMath::Abs(Item->WorldScale3D.X * Item->WorldScale3D.Y * Item->WorldScale3D.Z);
                if (ScaleFactor > KINDA_SMALL_NUMBER)
                {
                    Mass *= ScaleFactor;
                }

                if (Mass > KINDA_SMALL_NUMBER)
                {
                    return Mass;
                }
            }
        }

        if (USkeletalMesh* SkeletalMesh = Item->WorldSkeletalMesh.LoadSynchronous())
        {
            if (UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset())
            {
                float Mass = 0.f;
                for (USkeletalBodySetup* BodySetup : PhysicsAsset->SkeletalBodySetups)
                {
                    if (BodySetup)
                    {
                        Mass += BodySetup->CalculateMass();
                    }
                }

                if (Mass > KINDA_SMALL_NUMBER)
                {
                    return Mass;
                }
            }
        }

        return 0.f;
    }

    float ResolveItemVolumeM3(const UItemData* Item)
    {
        if (!Item)
        {
            return 0.f;
        }

        if (Item->VolumeOverride > 0.f)
        {
            return Item->VolumeOverride;
        }

        float VolumeCm3 = 0.f;

        if (UStaticMesh* StaticMesh = Item->WorldStaticMesh.LoadSynchronous())
        {
            if (UBodySetup* BodySetup = StaticMesh->GetBodySetup())
            {
                VolumeCm3 = BodySetup->AggGeom.GetScaledVolume(Item->WorldScale3D);
            }

            if (VolumeCm3 <= 0.f)
            {
                const FBoxSphereBounds Bounds = StaticMesh->GetBounds();
                const FVector Extents = Bounds.BoxExtent * Item->WorldScale3D * 2.f;
                VolumeCm3 = Extents.X * Extents.Y * Extents.Z;
            }
        }

        if (VolumeCm3 <= 0.f)
        {
            if (USkeletalMesh* SkeletalMesh = Item->WorldSkeletalMesh.LoadSynchronous())
            {
                const FBoxSphereBounds Bounds = SkeletalMesh->GetBounds();
                const FVector Extents = Bounds.BoxExtent * Item->WorldScale3D * 2.f;
                VolumeCm3 = Extents.X * Extents.Y * Extents.Z;
            }
        }

        return FMath::Max(0.f, VolumeCm3) * Cm3ToM3;
    }
}

float UInventoryComponent::GetTotalWeight() const
{
    double TotalWeight = 0.0;
    for (const FItemStack& Slot : Slots)
    {
        if (!Slot.Item || Slot.Quantity <= 0)
        {
            continue;
        }

        TotalWeight += static_cast<double>(ResolveItemWeightKg(Slot.Item)) * Slot.Quantity;
    }

    return static_cast<float>(TotalWeight);
}

float UInventoryComponent::GetTotalVolume() const
{
    double TotalVolume = 0.0;
    for (const FItemStack& Slot : Slots)
    {
        if (!Slot.Item || Slot.Quantity <= 0)
        {
            continue;
        }

        TotalVolume += static_cast<double>(ResolveItemVolumeM3(Slot.Item)) * Slot.Quantity;
    }

    return static_cast<float>(TotalVolume);
}

void UInventoryComponent::EnsurePersistentId()
{
    if (!PersistentId.IsValid())
    {
        PersistentId = FGuid::NewGuid();
    }
}

void UInventoryComponent::WriteToSaveData(FInventorySaveData& OutData) const
{
    OutData.MaxSlots = MaxSlots;
    OutData.MaxWeight = MaxWeight;
    OutData.MaxVolume = MaxVolume;

    OutData.Slots.SetNum(Slots.Num());
    for (int32 Index = 0; Index < Slots.Num(); ++Index)
    {
        const FItemStack& SourceSlot = Slots[Index];
        FInventorySlotSaveData& SaveSlot = OutData.Slots[Index];

        SaveSlot.ItemPath.Reset();
        SaveSlot.Quantity = FMath::Max(0, SourceSlot.Quantity);

        if (SourceSlot.Item)
        {
            SaveSlot.ItemPath = FSoftObjectPath(SourceSlot.Item.Get());
        }
    }
}

void UInventoryComponent::ReadFromSaveData(const FInventorySaveData& InData)
{
    MaxSlots = InData.MaxSlots > 0 ? InData.MaxSlots : MaxSlots;
    MaxWeight = InData.MaxWeight;
    MaxVolume = InData.MaxVolume;

    EnsureSlotCapacity();

    const int32 SlotCount = Slots.Num();
    for (int32 Index = 0; Index < SlotCount; ++Index)
    {
        FItemStack& TargetSlot = Slots[Index];
        if (InData.Slots.IsValidIndex(Index))
        {
            ResolveItemIntoSlot(InData.Slots[Index], TargetSlot);
        }
        else
        {
            TargetSlot.Item = nullptr;
            TargetSlot.Quantity = 0;
        }
    }

    OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::ResolveItemIntoSlot(const FInventorySlotSaveData& SlotData, FItemStack& Slot)
{
    Slot.Item = nullptr;
    Slot.Quantity = FMath::Max(0, SlotData.Quantity);

    if (SlotData.ItemPath.IsNull() || Slot.Quantity <= 0)
    {
        Slot.Quantity = 0;
        return;
    }

    UObject* LoadedObject = SlotData.ItemPath.TryLoad();
    if (UItemData* LoadedItem = Cast<UItemData>(LoadedObject))
    {
        Slot.Item = LoadedItem;
        const int32 MaxStack = Slot.MaxStack();
        if (MaxStack > 0)
        {
            Slot.Quantity = FMath::Clamp(Slot.Quantity, 0, MaxStack);
        }
    }
    else
    {
        Slot.Quantity = 0;
    }
}

bool UInventoryComponent::DropSingleItemInternal(int32 SlotIndex)
{
    EnsureSlotCapacity();

    if (!Slots.IsValidIndex(SlotIndex))
    {
        return false;
    }

    FItemStack& Slot = Slots[SlotIndex];
    if (Slot.IsEmpty())
    {
        return false;
    }

    UWorld* World = GetWorld();
    AActor* OwnerActor = GetOwner();
    if (!World || !OwnerActor)
    {
        return false;
    }

    UItemData* ItemData = Slot.Item;
    if (!ItemData)
    {
        return false;
    }

    UClass* PickupClass = nullptr;
    if (!ItemData->PickupActorClass.IsNull())
    {
        PickupClass = ItemData->PickupActorClass.LoadSynchronous();

        if (PickupClass && !PickupClass->IsChildOf(AItemPickup::StaticClass()))
        {
            UE_LOG(LogTemp, Warning, TEXT("PickupActorClass on %s is not an ItemPickup. Using default."), *ItemData->GetName());
            PickupClass = nullptr;
        }
    }

    if (!PickupClass)
    {
        PickupClass = AItemPickup::StaticClass();
    }

    constexpr float ForwardDropDistance = 100.f;
    constexpr float DropHeightOffset = 50.f;
    constexpr float DropSpreadRadius = 25.f;

    FVector SpawnLocation = OwnerActor->GetActorLocation() + OwnerActor->GetActorForwardVector() * ForwardDropDistance + FVector(0.f, 0.f, DropHeightOffset);

    const FVector2D RandomCircle = FMath::RandPointInCircle(DropSpreadRadius);
    SpawnLocation += FVector(RandomCircle.X, RandomCircle.Y, 0.f);
    const FRotator SpawnRotation = OwnerActor->GetActorRotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = OwnerActor;
    SpawnParams.Instigator = OwnerActor->GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AItemPickup* SpawnedPickup = World->SpawnActor<AItemPickup>(PickupClass, SpawnLocation, SpawnRotation, SpawnParams);
    if (!SpawnedPickup)
    {
        return false;
    }

    SpawnedPickup->SetItem(ItemData);
    SpawnedPickup->SetQuantity(1);
    SpawnedPickup->SetPersistentId(FGuid::NewGuid());
    SpawnedPickup->SetWasSpawnedFromInventory(true);
    SpawnedPickup->SetDropped(true);

    Slot.Quantity = FMath::Max(0, Slot.Quantity - 1);
    if (Slot.Quantity == 0)
    {
        Slot.Item = nullptr;
    }

    OnInventoryUpdated.Broadcast();
    return true;
}

void UInventoryComponent::HandleDropAllTimerTick(int32 SlotIndex)
{
    if (!DropSingleItemInternal(SlotIndex))
    {
        ClearDropAllTimer(SlotIndex);
        return;
    }

    EnsureSlotCapacity();
    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty())
    {
        ClearDropAllTimer(SlotIndex);
    }
}

void UInventoryComponent::ClearDropAllTimer(int32 SlotIndex)
{
    if (FTimerHandle* Handle = ActiveDropAllTimers.Find(SlotIndex))
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(*Handle);
        }

        ActiveDropAllTimers.Remove(SlotIndex);
    }
}

bool UInventoryComponent::TransferItemBetweenSlots(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
    EnsureSlotCapacity();

    if (SourceSlotIndex == TargetSlotIndex)
    {
        return false;
    }

    if (!Slots.IsValidIndex(SourceSlotIndex) || !Slots.IsValidIndex(TargetSlotIndex))
    {
        return false;
    }

    FItemStack& SourceSlot = Slots[SourceSlotIndex];
    if (SourceSlot.IsEmpty())
    {
        return false;
    }

    FItemStack& TargetSlot = Slots[TargetSlotIndex];
    bool bInventoryChanged = false;

    if (TargetSlot.IsEmpty())
    {
        TargetSlot = SourceSlot;
        SourceSlot.Item = nullptr;
        SourceSlot.Quantity = 0;
        bInventoryChanged = true;
    }
    else if (TargetSlot.Item == SourceSlot.Item)
    {
        const int32 MaxStackSize = TargetSlot.MaxStack();
        if (MaxStackSize > 0)
        {
            const int32 SpaceAvailable = FMath::Max(0, MaxStackSize - TargetSlot.Quantity);
            if (SpaceAvailable > 0)
            {
                const int32 TransferAmount = FMath::Min(SpaceAvailable, SourceSlot.Quantity);
                TargetSlot.Quantity += TransferAmount;
                SourceSlot.Quantity -= TransferAmount;
                bInventoryChanged = TransferAmount > 0;
            }

            if (SourceSlot.Quantity <= 0)
            {
                SourceSlot.Quantity = 0;
                SourceSlot.Item = nullptr;
            }
        }
    }
    else
    {
        Swap(SourceSlot, TargetSlot);
        bInventoryChanged = true;
    }

    if (bInventoryChanged)
    {
        OnInventoryUpdated.Broadcast();
    }

    return bInventoryChanged;
}

void UInventoryComponent::EnsureSlotCapacity()
{
    const int32 DesiredSlots = FMath::Max(1, MaxSlots);
    if (Slots.Num() != DesiredSlots)
    {
        Slots.SetNum(DesiredSlots);
    }
}

#if WITH_EDITOR
void UInventoryComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property &&
        PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UInventoryComponent, MaxSlots))
    {
        EnsureSlotCapacity();
    }
}
#endif
