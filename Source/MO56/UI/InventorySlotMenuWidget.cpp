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
        const FSlateBrush* BackgroundBrush = &FAppStyle::Get().GetBrush("Menu.Background");

        TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(4.f)
                [
                        SNew(SButton)
                        .Text(NSLOCTEXT("Inventory", "SplitStack", "Split Stack"))
                        .OnClicked(FOnClicked::CreateUObject(this, &UInventorySlotMenuWidget::HandleSplitStackClicked))
                        .IsEnabled(this, &UInventorySlotMenuWidget::CanSplitStack)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(4.f)
                [
                        SNew(SButton)
                        .Text(NSLOCTEXT("Inventory", "DestroyItem", "Destroy Item"))
                        .OnClicked(FOnClicked::CreateUObject(this, &UInventorySlotMenuWidget::HandleDestroyItemClicked))
                        .IsEnabled(this, &UInventorySlotMenuWidget::CanDestroyItem)
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

        if (UInventorySlotWidget* Slot = OwningSlot.Get())
        {
                Slot->NotifyContextMenuClosed(this);
        }

        OwningSlot.Reset();
}

FReply UInventorySlotMenuWidget::HandleSplitStackClicked()
{
        if (UInventorySlotWidget* Slot = OwningSlot.Get())
        {
                if (Slot->HandleSplitStack())
                {
                        DismissMenu();
                }
        }

        return FReply::Handled();
}

FReply UInventorySlotMenuWidget::HandleDestroyItemClicked()
{
        if (UInventorySlotWidget* Slot = OwningSlot.Get())
        {
                if (Slot->HandleDestroyItem())
                {
                        DismissMenu();
                }
        }

        return FReply::Handled();
}

bool UInventorySlotMenuWidget::CanSplitStack() const
{
        if (const UInventorySlotWidget* Slot = OwningSlot.Get())
        {
                return Slot->CanSplitStack();
        }

        return false;
}

bool UInventorySlotMenuWidget::CanDestroyItem() const
{
        if (const UInventorySlotWidget* Slot = OwningSlot.Get())
        {
                return Slot->HasItem();
        }

        return false;
}

