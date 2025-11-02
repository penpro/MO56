#pragma once

#include "Blueprint/UserWidget.h"
#include "InventorySlotDragVisual.generated.h"

class UImage;

struct FItemStack;

/**
 * Drag preview widget used while dragging inventory items.
 *
 * Editor Implementation Guide:
 * 1. Create a Blueprint subclass and design the drag icon (background, stack count, rarity frame).
 * 2. Bind ItemIcon to an Image widget that displays the item's texture supplied via SetDraggedStack.
 * 3. Optionally add a text block or overlay to show quantity or weight while dragging.
 * 4. Assign this Blueprint in UInventorySlotWidget::DragVisualClass so it spawns during drag detection.
 * 5. Test in PIE to ensure the drag visual follows the cursor and clears when the drag operation ends.
 */
UCLASS()
class MO56_API UInventorySlotDragVisual : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetDraggedStack(const FItemStack& Stack);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon;
};
