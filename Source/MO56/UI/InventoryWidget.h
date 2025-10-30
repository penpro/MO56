#pragma once

#include "Blueprint/UserWidget.h"
#include "InventoryUpdateInterface.h"
#include "InventoryWidget.generated.h"

class APawn;
class UInventoryComponent;
class UInventorySlotWidget;
class UPanelWidget;

/**
 * Widget that displays the contents of an inventory component.
 */
UCLASS()
class MO56_API UInventoryWidget : public UUserWidget, public IInventoryUpdateInterface
{
        GENERATED_BODY()

public:
        UInventoryWidget(const FObjectInitializer& ObjectInitializer);

        virtual void NativeConstruct() override;
        virtual void NativeDestruct() override;

        /** Explicitly sets the inventory component to observe. */
        UFUNCTION(BlueprintCallable, Category = "Inventory")
        void SetInventoryComponent(UInventoryComponent* NewInventory);

        virtual void OnUpdateInventory_Implementation(UInventoryComponent* Inventory) override;

protected:
        /** Container that holds the generated inventory slot widgets. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> SlotsContainer;

        /** Widget class used for each inventory slot. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
        TSubclassOf<UInventorySlotWidget> SlotWidgetClass;

        /** Whether the widget should automatically track the owning controller's pawn inventory. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
        bool bAutoBindToOwningPawn = true;

private:
        void ObserveOwningPlayer();
        void StopObservingOwningPlayer();
        void BindToInventoryFromPawn(APawn* Pawn);
        void HandlePawnChanged(APawn* NewPawn);
        void RefreshInventory(UInventoryComponent* Inventory);

        UFUNCTION()
        void HandleInventoryComponentUpdated();

        TWeakObjectPtr<UInventoryComponent> ObservedInventory;
        FDelegateHandle PawnChangedHandle;
};

