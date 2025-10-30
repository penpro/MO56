#include "UI/InventorySlotMenuWidget.h"

#include "UI/InventorySlotWidget.h"

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

