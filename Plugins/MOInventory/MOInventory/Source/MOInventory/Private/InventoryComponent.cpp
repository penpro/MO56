#include "InventoryComponent.h"
#include "ItemData.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    EnsureSlotCapacity();
}

void UInventoryComponent::InitializeComponent()
{
    Super::InitializeComponent();
    EnsureSlotCapacity();
}

int32 FItemStack::MaxStack() const
{
    if (!Item) return 0;
    return FMath::Max(1, Item->MaxStackSize);
}

int32 UInventoryComponent::AddItem(UItemData* Item, int32 Count)
{
    if (!Item || Count <= 0) return 0;
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
