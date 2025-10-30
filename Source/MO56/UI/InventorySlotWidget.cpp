#include "UI/InventorySlotWidget.h"

#include "UI/InventorySlotMenuWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "InventoryComponent.h"
#include "ItemData.h"
#include "InputCoreTypes.h"

UInventorySlotWidget::UInventorySlotWidget(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{
        ContextMenuClass = UInventorySlotMenuWidget::StaticClass();
}

void UInventorySlotWidget::SetItemStack(const FItemStack& Stack)
{
        CachedStack = Stack;

        if (ItemIcon)
        {
                if (Stack.Item)
                {
                        if (UTexture2D* IconTexture = Stack.Item->Icon.LoadSynchronous())
                        {
                                ItemIcon->SetBrushFromTexture(IconTexture);
                                ItemIcon->SetVisibility(ESlateVisibility::Visible);
                        }
                        else
                        {
                                ItemIcon->SetBrushFromTexture(nullptr);
                                ItemIcon->SetVisibility(ESlateVisibility::Hidden);
                        }
                }
                else
                {
                        ItemIcon->SetBrushFromTexture(nullptr);
                        ItemIcon->SetVisibility(ESlateVisibility::Hidden);
                }
        }

        if (QuantityText)
        {
                if (Stack.Quantity > 1)
                {
                        QuantityText->SetText(FText::AsNumber(Stack.Quantity));
                        QuantityText->SetVisibility(ESlateVisibility::Visible);
                }
                else if (Stack.Item)
                {
                        QuantityText->SetText(FText::GetEmpty());
                        QuantityText->SetVisibility(ESlateVisibility::Hidden);
                }
                else
                {
                        QuantityText->SetText(FText::GetEmpty());
                        QuantityText->SetVisibility(ESlateVisibility::Hidden);
                }

                if (QuantityBadge)
                {
                        const bool bShowBadge = (Stack.Item != nullptr) && (Stack.Quantity > 1);
                        QuantityBadge->SetVisibility(bShowBadge ? ESlateVisibility::Visible
                                                                : ESlateVisibility::Collapsed);
                }
                else if (QuantityText)
                {
                        // Fallback if the SizeBox wasn’t bound
                        const bool bShow = (Stack.Item != nullptr) && (Stack.Quantity > 1);
                        QuantityText->SetVisibility(bShow ? ESlateVisibility::Visible
                                                          : ESlateVisibility::Collapsed);
                }
        }

        if (CachedStack.IsEmpty())
        {
                CloseContextMenu();
        }
}

void UInventorySlotWidget::InitializeSlot(UInventoryComponent* Inventory, int32 InSlotIndex)
{
        ObservedInventory = Inventory;
        SlotIndex = InSlotIndex;
}

bool UInventorySlotWidget::CanSplitStack() const
{
        return CachedStack.Item != nullptr && CachedStack.Quantity > 1;
}

bool UInventorySlotWidget::HasItem() const
{
        return CachedStack.Item != nullptr && CachedStack.Quantity > 0;
}

bool UInventorySlotWidget::HandleSplitStack()
{
        if (!ObservedInventory.IsValid() || SlotIndex == INDEX_NONE)
        {
                return false;
        }

        return ObservedInventory->SplitStackAtIndex(SlotIndex);
}

bool UInventorySlotWidget::HandleDestroyItem()
{
        if (!ObservedInventory.IsValid() || SlotIndex == INDEX_NONE)
        {
                return false;
        }

        return ObservedInventory->DestroyItemAtIndex(SlotIndex);
}

void UInventorySlotWidget::CloseContextMenu()
{
        if (UInventorySlotMenuWidget* Menu = ActiveContextMenu.Get())
        {
                Menu->DismissMenu();
        }
}

void UInventorySlotWidget::NotifyContextMenuClosed(UInventorySlotMenuWidget* ClosedMenu)
{
        if (ActiveContextMenu.Get() == ClosedMenu)
        {
                ActiveContextMenu = nullptr;
        }
}

void UInventorySlotWidget::NativeDestruct()
{
        CloseContextMenu();
        Super::NativeDestruct();
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
        if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
        {
                ShowContextMenu(InMouseEvent.GetScreenSpacePosition());
                return FReply::Handled();
        }

        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::ShowContextMenu(const FVector2D& ScreenPosition)
{
        if (!HasItem())
        {
                CloseContextMenu();
                return;
        }

        if (!ContextMenuClass)
        {
                return;
        }

        if (UInventorySlotMenuWidget* ExistingMenu = ActiveContextMenu.Get())
        {
                ExistingMenu->DismissMenu();
        }

        UWorld* World = GetWorld();
        if (!World)
        {
                return;
        }

        APlayerController* OwningPlayer = GetOwningPlayer();

        UInventorySlotMenuWidget* Menu = nullptr;
        if (OwningPlayer)
        {
                Menu = CreateWidget<UInventorySlotMenuWidget>(OwningPlayer, ContextMenuClass);
        }
        else
        {
                Menu = CreateWidget<UInventorySlotMenuWidget>(World, ContextMenuClass);
        }
        if (!Menu)
        {
                return;
        }

        Menu->SetOwningSlot(this);
        Menu->AddToViewport(1000);

        float ViewportScale = 1.f;
        if (OwningPlayer)
        {
                ViewportScale = UWidgetLayoutLibrary::GetViewportScale(OwningPlayer);
        }
        else if (APlayerController* PC = World->GetFirstPlayerController())
        {
                ViewportScale = UWidgetLayoutLibrary::GetViewportScale(PC);
        }

        const FVector2D LocalPosition = ScreenPosition / ViewportScale;
        Menu->SetPositionInViewport(LocalPosition, false);
        ActiveContextMenu = Menu;
}

