#include "UI/InventorySlotDragOperation.h"

#include "InventoryComponent.h"

void UInventorySlotDragOperation::InitializeOperation(UInventoryComponent* InInventory, int32 InSourceSlotIndex, const FItemStack& InStack)
{
        SourceInventory = InInventory;
        SourceSlotIndex = InSourceSlotIndex;
        DraggedStack = InStack;
}
