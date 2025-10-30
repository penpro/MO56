#pragma once

#include "Blueprint/UserWidget.h"
#include "InventorySlotDragVisual.generated.h"

class UImage;

struct FItemStack;

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
