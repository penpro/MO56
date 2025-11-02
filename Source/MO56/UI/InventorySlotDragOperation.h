#pragma once

#include "InventoryComponent.h"
#include "Blueprint/DragDropOperation.h"
#include "InventorySlotDragOperation.generated.h"

/**
 * Drag-drop operation carrying inventory slot metadata during UI drags.
 *
 * Editor Implementation Guide:
 * 1. Use this class as the DragDropOperation in your InventorySlotWidget Blueprint when initiating a drag.
 * 2. Call InitializeOperation in Blueprint to capture the source inventory, slot index, and stack before starting the drag.
 * 3. In drop targets (inventory grids, world drop surfaces) cast the operation to UInventorySlotDragOperation to access data.
 * 4. Pair with UInventorySlotDragVisual to display the dragged stack while the operation is active.
 * 5. Extend this class in Blueprint if you need to add additional payload (e.g., source container actor or permissions).
 */
UCLASS()
class MO56_API UInventorySlotDragOperation : public UDragDropOperation
{
        GENERATED_BODY()

public:
        void InitializeOperation(UInventoryComponent* InInventory, int32 InSourceSlotIndex, const FItemStack& InStack);

        UFUNCTION(BlueprintPure, Category = "Inventory")
        UInventoryComponent* GetSourceInventory() const { return SourceInventory.Get(); }

        UFUNCTION(BlueprintPure, Category = "Inventory")
        int32 GetSourceSlotIndex() const { return SourceSlotIndex; }

        const FItemStack& GetDraggedStack() const { return DraggedStack; }

private:
        TWeakObjectPtr<UInventoryComponent> SourceInventory;
        int32 SourceSlotIndex = INDEX_NONE;

        UPROPERTY()
        FItemStack DraggedStack;
};
