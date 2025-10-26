#include "InventoryComponent.h"

// If your ItemData header lives at a different include path, adjust this:
// e.g. #include "MOItems/Public/ItemData.h"
#include "ItemData.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    Slots.SetNum(MaxSlots);
}

int32 FItemStack::MaxStack() const
{
    if (!Item) return 0;

    // If your UItemData exposes MaxStackSize, use it; else default to 1.
    // Replace "MaxStackSize" below with your actual property name if different.
    const int32* PropPtr = nullptr; // placeholder for compile aid
    const int32 MaxFromItem =
#if __has_include("ItemData.h")
        // If Item has MaxStackSize, this will compile. If not, comment next line.
        Item->MaxStackSize
#else
        1
#endif
        ;
    return FMath::Max(1, MaxFromItem);
}

int32 UInventoryComponent::AddItem(UItemData* Item, int32 Count)
{
    if (!Item || Count <= 0) return 0;
    int32 Remaining = Count;
    Remaining -= AddToExistingStacks(Item, Remaining);
    Remaining -= AddToEmptySlots(Item, Remaining);
    return Count - Remaining;
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
            if (Slot.Quantity <= 0) { Slot.Quantity = 0; Slot.Item = nullptr; }
        }
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

int32 UInventoryComponent::RemoveItemByAssetName(FName AssetName, int32 Count)
{
    if (AssetName.IsNone() || Count <= 0) return 0;
    int32 Removed = 0;
    for (FItemStack& Slot : Slots)
    {
        if (Count <= 0) break;
        if (Slot.Item && Slot.Item->GetFName() == AssetName)
        {
            const int32 ToRemove = FMath::Min(Slot.Quantity, Count);
            Slot.Quantity -= ToRemove;
            Removed += ToRemove;
            Count -= ToRemove;
            if (Slot.Quantity <= 0) { Slot.Quantity = 0; Slot.Item = nullptr; }
        }
    }
    return Removed;
}

int32 UInventoryComponent::CountItemByAssetName(FName AssetName) const
{
    if (AssetName.IsNone()) return 0;
    int32 Total = 0;
    for (const FItemStack& Slot : Slots)
    {
        if (Slot.Item && Slot.Item->GetFName() == AssetName) { Total += Slot.Quantity; }
    }
    return Total;
}
