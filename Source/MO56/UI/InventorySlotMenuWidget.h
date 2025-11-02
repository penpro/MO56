#pragma once


#include "Input/Reply.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlotMenuWidget.generated.h"



class UInventorySlotWidget;
struct FPointerEvent;

/**
 * Simple context menu displayed when right-clicking an inventory slot.
 *
 * Editor Implementation Guide:
 * 1. Create a Blueprint subclass and design the menu layout (buttons for split/drop/destroy/inspect).
 * 2. Bind button events to call the corresponding Handle* functions or override them in Blueprint for custom logic.
 * 3. When showing the menu, call SetOwningSlot from UInventorySlotWidget to grant access to slot data/actions.
 * 4. Use DismissMenu to close the widget before removing it from viewport, optionally playing an outro animation.
 * 5. Ensure the menu blueprint is assigned to UInventorySlotWidget::ContextMenuClass so slots spawn it on demand.
 */
UCLASS()
class MO56_API UInventorySlotMenuWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        /** Assigns the slot widget that spawned this menu. */
        void SetOwningSlot(UInventorySlotWidget* InOwningSlot);

        /** Closes the menu and removes it from the viewport. */
        void DismissMenu();

protected:
        virtual TSharedRef<SWidget> RebuildWidget() override;
        virtual void ReleaseSlateResources(bool bReleaseChildren) override;
        virtual void NativeDestruct() override;
        virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
        /** Handles the "Split Stack" button being clicked. */
        FReply HandleSplitStackClicked();

        /** Handles the "Inspect" button being clicked. */
        FReply HandleInspectClicked();

        /** Handles the "Destroy Item" button being clicked. */
        FReply HandleDestroyItemClicked();

        /** Handles the "Drop All" button being clicked. */
        FReply HandleDropAllItemsClicked();

        /** Handles the "Drop One" button being clicked. */
        FReply HandleDropOneItemClicked();

        /** Whether the owning slot can currently split its stack. */
        bool CanSplitStack() const;

        /** Whether the owning slot currently contains an item. */
        bool CanDestroyItem() const;

        /** Whether the owning slot can drop any items. */
        bool CanDropItems() const;

        /** Whether the owning slot supports inspection. */
        bool CanInspectItem() const;

        /** Shared reference to the root Slate widget. */
        TSharedPtr<SWidget> RootWidget;

        /** Slot widget that spawned this menu. */
        TWeakObjectPtr<UInventorySlotWidget> OwningSlot;
};

