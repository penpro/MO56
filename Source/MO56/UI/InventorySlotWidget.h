#pragma once
#include "InventoryComponent.h"
#include "Templates/SubclassOf.h"
#include "Input/Reply.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlotWidget.generated.h"

class UImage;
class UTextBlock;
class USizeBox;
class UInventoryComponent;
class UInventorySlotMenuWidget;

/**
 * Simple widget representing a single inventory slot.
 */
UCLASS()
class MO56_API UInventorySlotWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        UInventorySlotWidget(const FObjectInitializer& ObjectInitializer);

        /** Displays the supplied item stack information. */
        UFUNCTION(BlueprintCallable, Category = "Inventory")
        void SetItemStack(const FItemStack& Stack);

        /**
         * Assigns the inventory component and slot index that this widget represents.
         */
        void InitializeSlot(UInventoryComponent* Inventory, int32 InSlotIndex);

        /** Returns true if the slot currently contains an item stack that can be split. */
        bool CanSplitStack() const;

        /** Returns true if the slot currently has any item. */
        bool HasItem() const;

        /** Attempts to split the current stack in half. */
        bool HandleSplitStack();

        /** Destroys the item contained in the slot. */
        bool HandleDestroyItem();

        /** Closes any active context menu. */
        void CloseContextMenu();

        /** Notifies the slot that its context menu has been closed. */
        void NotifyContextMenuClosed(UInventorySlotMenuWidget* ClosedMenu);

protected:
        virtual void NativeDestruct() override;
        virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

        void ShowContextMenu(const FVector2D& ScreenPosition);

        /** Image used to display the item's icon. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UImage> ItemIcon;

        /** Text block used to display the item quantity. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> QuantityText;

        // Bind the badge container
        UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
        TObjectPtr<USizeBox> QuantityBadge;

        /** Class used to spawn the slot context menu. */
        UPROPERTY(EditAnywhere, Category = "Inventory")
        TSubclassOf<UInventorySlotMenuWidget> ContextMenuClass;

        /** Inventory component providing data for this slot. */
        TWeakObjectPtr<UInventoryComponent> ObservedInventory;

        /** Cached stack data for the slot. */
        FItemStack CachedStack;

        /** Index of the slot within the observed inventory. */
        int32 SlotIndex = INDEX_NONE;

        /** Currently spawned context menu widget, if any. */
        TWeakObjectPtr<UInventorySlotMenuWidget> ActiveContextMenu;
};

