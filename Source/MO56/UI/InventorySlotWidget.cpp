#include "UI/InventorySlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "InventoryComponent.h"
#include "ItemData.h"

void UInventorySlotWidget::SetItemStack(const FItemStack& Stack)
{
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
        }
}

