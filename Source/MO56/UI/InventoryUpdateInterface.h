#pragma once

#include "UObject/Interface.h"
#include "InventoryUpdateInterface.generated.h"

class UInventoryComponent;

/**
 * Interface implemented by widgets that react to inventory change notifications.
 *
 * Editor Implementation Guide:
 * 1. Create a UUserWidget Blueprint and implement the InventoryUpdateInterface in the Class Settings panel.
 * 2. Override OnUpdateInventory in the Blueprint graph to rebuild slot widgets from the supplied component.
 * 3. Use UInventoryComponent::OnInventoryChanged to call OnUpdateInventory on any bound widget instances.
 * 4. Combine with UInventorySlotWidget blueprints to visually represent each stack in the refreshed layout.
 * 5. Remember to remove bindings in Destruct so widgets do not reference destroyed inventory components.
 */
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

