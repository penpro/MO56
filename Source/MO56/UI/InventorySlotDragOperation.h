#pragma once

#include "InventoryComponent.h"
#include "Blueprint/DragDropOperation.h"
#include "InventorySlotDragOperation.generated.h"

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
