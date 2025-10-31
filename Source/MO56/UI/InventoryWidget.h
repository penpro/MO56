#pragma once

#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Blueprint/UserWidget.h"
#include "InventoryUpdateInterface.h"
#include "InventoryWidget.generated.h"

class APawn;
class UInventoryComponent;
class UInventorySlotWidget;
class UPanelWidget;
class UTextBlock;

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

        /** Enables or disables automatically binding to the owning pawn's inventory. */
        UFUNCTION(BlueprintCallable, Category = "Inventory")
        void SetAutoBindToOwningPawn(bool bEnabled);

        virtual void OnUpdateInventory_Implementation(UInventoryComponent* Inventory) override;

protected:
        /** Container that holds the generated inventory slot widgets. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        UUniformGridPanel* SlotsContainer = nullptr;

        /** Displays the total weight stored in the inventory. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        UTextBlock* WeightTotal = nullptr;

        /** Displays the total volume stored in the inventory. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        UTextBlock* VolumeTotal = nullptr;

        /** Displays the maximum weight capacity of the inventory. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        UTextBlock* MaxWeight = nullptr;

        /** Displays the maximum volume capacity of the inventory. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        UTextBlock* MaxVolume = nullptr;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
        int32 Columns = 6;

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
        void UpdateInventoryStats(UInventoryComponent* Inventory);

        UFUNCTION()
        void HandleInventoryComponentUpdated();

        TWeakObjectPtr<UInventoryComponent> ObservedInventory;
        FDelegateHandle PawnChangedHandle;
};

