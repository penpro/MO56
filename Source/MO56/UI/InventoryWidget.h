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
class USkillSystemComponent;

/**
 * Widget that displays the contents of an inventory component.
 *
 * Editor Implementation Guide:
 * 1. Create a Blueprint subclass and design the inventory grid using a UniformGridPanel bound to SlotsContainer.
 * 2. Assign SlotWidgetClass to your UInventorySlotWidget Blueprint so dynamic slot widgets spawn correctly.
 * 3. Bind Weight/Volume text blocks to display aggregated stats via UpdateInventoryStats.
 * 4. Set bAutoBindToOwningPawn when the widget lives in the HUD so it auto-discovers the player's inventory component.
 * 5. Implement OnUpdateInventory in Blueprint (or rely on the C++ default) to refresh slot visuals whenever notified.
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

        void SetSkillSystemComponent(USkillSystemComponent* SkillSystem);

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
        TWeakObjectPtr<USkillSystemComponent> ObservedSkillSystem;
        FDelegateHandle PawnChangedHandle;
};

