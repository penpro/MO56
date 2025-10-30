#include "UI/InventorySlotMenuWidget.h"

#include "UI/InventorySlotWidget.h"

#include "Input/Events.h"
#include "Internationalization/Text.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void UInventorySlotMenuWidget::SetOwningSlot(UInventorySlotWidget* InOwningSlot)
{
        OwningSlot = InOwningSlot;
}

void UInventorySlotMenuWidget::DismissMenu()
{
        RemoveFromParent();
}

TSharedRef<SWidget> UInventorySlotMenuWidget::RebuildWidget()
{
	const FSlateBrush* BackgroundBrush = FAppStyle::Get().GetBrush("Menu.Background");

        TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(4.f)
                [
                        SNew(SButton)
                        .Text(NSLOCTEXT("Inventory", "SplitStack", "Split Stack"))
                        .OnClicked(FOnClicked::CreateUObject(this, &UInventorySlotMenuWidget::HandleSplitStackClicked))
                        .IsEnabled_UObject(this, &UInventorySlotMenuWidget::CanSplitStack)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(4.f)
                [
                        SNew(SButton)
                        .Text(NSLOCTEXT("Inventory", "DestroyItem", "Destroy Item"))
                        .OnClicked(FOnClicked::CreateUObject(this, &UInventorySlotMenuWidget::HandleDestroyItemClicked))
                        .IsEnabled_UObject(this, &UInventorySlotMenuWidget::CanDestroyItem)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(4.f)
                [
                        SNew(SButton)
                        .Text(NSLOCTEXT("Inventory", "DropItem", "Drop Item"))
                        .OnClicked(FOnClicked::CreateUObject(this, &UInventorySlotMenuWidget::HandleDropItemClicked))
                        .IsEnabled_UObject(this, &UInventorySlotMenuWidget::CanDropItem)
                ];

        TSharedRef<SWidget> Result =
                SAssignNew(RootWidget, SBorder)
                .BorderImage(BackgroundBrush)
                .Padding(4.f)
                [
                        MenuContent
                ];

        return Result;
}

void UInventorySlotMenuWidget::ReleaseSlateResources(bool bReleaseChildren)
{
        Super::ReleaseSlateResources(bReleaseChildren);
        RootWidget.Reset();
}

void UInventorySlotMenuWidget::NativeDestruct()
{
        Super::NativeDestruct();

        if (UInventorySlotWidget* SlotWidget = OwningSlot.Get())
        {
                SlotWidget->NotifyContextMenuClosed(this);
        }

        OwningSlot.Reset();
}

void UInventorySlotMenuWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
        Super::NativeOnMouseLeave(InMouseEvent);

        DismissMenu();
}

FReply UInventorySlotMenuWidget::HandleSplitStackClicked()
{
        if (UInventorySlotWidget* SlotWidget = OwningSlot.Get())
        {
                if (SlotWidget->HandleSplitStack())
                {
                        DismissMenu();
                }
        }

        return FReply::Handled();
}

FReply UInventorySlotMenuWidget::HandleDestroyItemClicked()
{
        if (UInventorySlotWidget* SlotWidget = OwningSlot.Get())
        {
                if (SlotWidget->HandleDestroyItem())
                {
                        DismissMenu();
                }
        }

        return FReply::Handled();
}

FReply UInventorySlotMenuWidget::HandleDropItemClicked()
{
        if (UInventorySlotWidget* SlotWidget = OwningSlot.Get())
        {
                if (SlotWidget->HandleDropItem())
                {
                        DismissMenu();
                }
        }

        return FReply::Handled();
}

bool UInventorySlotMenuWidget::CanSplitStack() const
{
        if (const UInventorySlotWidget* SlotWidget = OwningSlot.Get())
        {
                return SlotWidget->CanSplitStack();
        }

        return false;
}

bool UInventorySlotMenuWidget::CanDestroyItem() const
{
        if (const UInventorySlotWidget* SlotWidget = OwningSlot.Get())
        {
                return SlotWidget->HasItem();
        }

        return false;
}

bool UInventorySlotMenuWidget::CanDropItem() const
{
        if (const UInventorySlotWidget* SlotWidget = OwningSlot.Get())
        {
                return SlotWidget->HasItem();
        }

        return false;
}

