#pragma once
#include "InventoryComponent.h"
#include "Templates/SubclassOf.h"
#include "Input/DragAndDrop.h"
#include "Input/Reply.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlotWidget.generated.h"

class UImage;
class UTextBlock;
class USizeBox;
class UInventoryComponent;
class UInventorySlotMenuWidget;
class UInventorySlotDragVisual;
class UDragDropOperation;
class USkillSystemComponent;

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

        /** Ensures the owning player controller has authority over the supplied inventory before mutating it. */
        void EnsureInventoryOwnership(UInventoryComponent* Inventory) const;

        /** Returns true if the slot currently contains an item stack that can be split. */
        bool CanSplitStack() const;

        /** Returns true if the slot currently has any item. */
        bool HasItem() const;

        /** Attempts to split the current stack in half. */
        bool HandleSplitStack();

        /** Destroys the item contained in the slot. */
        bool HandleDestroyItem();

        /** Drops every item in the stack into the world. */
        bool HandleDropAllItems();

        /** Drops a single item from the stack into the world. */
        bool HandleDropOneItem();

        /** Starts inspecting the item contained in this slot. */
        bool HandleInspectItem();

        /** Whether the slot currently supports inspection. */
        bool CanInspectItem() const;

        /** Closes any active context menu. */
        void CloseContextMenu();

        /** Notifies the slot that its context menu has been closed. */
        void NotifyContextMenuClosed(UInventorySlotMenuWidget* ClosedMenu);

        void SetSkillSystem(USkillSystemComponent* InSkillSystem);

protected:
        virtual void NativeDestruct() override;
        virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
        virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
        virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

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

	/** Class used to create the drag drop visual while moving items. */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInventorySlotDragVisual> DragVisualClass;

        /** Inventory component providing data for this slot. */
        TWeakObjectPtr<UInventoryComponent> ObservedInventory;

        /** Cached stack data for the slot. */
        FItemStack CachedStack;

        /** Index of the slot within the observed inventory. */
        int32 SlotIndex = INDEX_NONE;

        /** Currently spawned context menu widget, if any. */
        TWeakObjectPtr<UInventorySlotMenuWidget> ActiveContextMenu;

        TWeakObjectPtr<USkillSystemComponent> SkillSystem;

};

