#pragma once

#include "UObject/Interface.h"
#include "InventoryUpdateInterface.generated.h"

class UInventoryComponent;

UINTERFACE(BlueprintType)
class MO56_API UInventoryUpdateInterface : public UInterface
{
        GENERATED_BODY()
};

class MO56_API IInventoryUpdateInterface
{
        GENERATED_BODY()

public:
        /** Called when the bound inventory should refresh its displayed contents. */
        UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
        void OnUpdateInventory(UInventoryComponent* Inventory);
};

