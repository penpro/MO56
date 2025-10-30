#include "UI/InventorySlotDragVisual.h"

#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "ItemData.h"
#include "InventoryComponent.h"

void UInventorySlotDragVisual::SetDraggedStack(const FItemStack& Stack)
{
	if (!ItemIcon)
	{
		return;
	}

	if (Stack.Item)
	{
		if (UTexture2D* IconTexture = Stack.Item->Icon.LoadSynchronous())
		{
			ItemIcon->SetBrushFromTexture(IconTexture);
			ItemIcon->SetVisibility(ESlateVisibility::Visible);
			return;
		}
	}

	ItemIcon->SetBrushFromTexture(nullptr);
	ItemIcon->SetVisibility(ESlateVisibility::Hidden);
}
