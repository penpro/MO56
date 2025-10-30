#pragma once

#include "Blueprint/UserWidget.h"
#include "InventorySlotWidget.generated.h"

struct FItemStack;
class UImage;
class UTextBlock;

/**
 * Simple widget representing a single inventory slot.
 */
UCLASS()
class MO56_API UInventorySlotWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        /** Displays the supplied item stack information. */
        UFUNCTION(BlueprintCallable, Category = "Inventory")
        void SetItemStack(const FItemStack& Stack);

protected:
        /** Image used to display the item's icon. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UImage> ItemIcon;

        /** Text block used to display the item quantity. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> QuantityText;
};

