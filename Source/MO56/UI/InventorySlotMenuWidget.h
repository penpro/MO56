#pragma once


#include "Input/Reply.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlotMenuWidget.generated.h"



class UInventorySlotWidget;
struct FPointerEvent;

/**
 * Simple context menu displayed when right-clicking an inventory slot.
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

        /** Handles the "Destroy Item" button being clicked. */
        FReply HandleDestroyItemClicked();

        /** Whether the owning slot can currently split its stack. */
        bool CanSplitStack() const;

        /** Whether the owning slot currently contains an item. */
        bool CanDestroyItem() const;

        /** Shared reference to the root Slate widget. */
        TSharedPtr<SWidget> RootWidget;

        /** Slot widget that spawned this menu. */
        TWeakObjectPtr<UInventorySlotWidget> OwningSlot;
};

