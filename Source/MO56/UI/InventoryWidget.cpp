#include "UI/InventoryWidget.h"

#include "Components/PanelWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "InventoryComponent.h"
#include "UI/InventorySlotWidget.h"



UInventoryWidget::UInventoryWidget(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{
        SlotWidgetClass = UInventorySlotWidget::StaticClass();
}

void UInventoryWidget::NativeConstruct()
{
        Super::NativeConstruct();

        if (bAutoBindToOwningPawn)
        {
                ObserveOwningPlayer();
                BindToInventoryFromPawn(GetOwningPlayerPawn());
        }
}

void UInventoryWidget::NativeDestruct()
{
        StopObservingOwningPlayer();
        SetInventoryComponent(nullptr);

        Super::NativeDestruct();
}

void UInventoryWidget::SetInventoryComponent(UInventoryComponent* NewInventory)
{
        if (ObservedInventory.Get() == NewInventory)
        {
                return;
        }

        if (UInventoryComponent* CurrentInventory = ObservedInventory.Get())
        {
                CurrentInventory->OnInventoryUpdated.RemoveDynamic(this, &UInventoryWidget::HandleInventoryComponentUpdated);
        }

        ObservedInventory = NewInventory;

        if (UInventoryComponent* Inventory = ObservedInventory.Get())
        {
                Inventory->OnInventoryUpdated.AddDynamic(this, &UInventoryWidget::HandleInventoryComponentUpdated);
        }

        IInventoryUpdateInterface::Execute_OnUpdateInventory(this, ObservedInventory.Get());
}

void UInventoryWidget::OnUpdateInventory_Implementation(UInventoryComponent* Inventory)
{
        RefreshInventory(Inventory);
}

void UInventoryWidget::ObserveOwningPlayer()
{
        if (APlayerController* PC = GetOwningPlayer())
        {
                StopObservingOwningPlayer();
                PawnChangedHandle = PC->GetOnNewPawnNotifier().AddUObject(this, &UInventoryWidget::HandlePawnChanged);
        }
}

void UInventoryWidget::StopObservingOwningPlayer()
{
        if (PawnChangedHandle.IsValid())
        {
                if (APlayerController* PC = GetOwningPlayer())
                {
                        PC->GetOnNewPawnNotifier().Remove(PawnChangedHandle);
                }

                PawnChangedHandle.Reset();
        }
}

void UInventoryWidget::BindToInventoryFromPawn(APawn* Pawn)
{
        if (!Pawn)
        {
                SetInventoryComponent(nullptr);
                return;
        }

        SetInventoryComponent(Pawn->FindComponentByClass<UInventoryComponent>());
}

void UInventoryWidget::HandlePawnChanged(APawn* NewPawn)
{
        BindToInventoryFromPawn(NewPawn);
}

void UInventoryWidget::RefreshInventory(UInventoryComponent* Inventory)
{
        if (!SlotsContainer)
                return;

        SlotsContainer->ClearChildren();
        SlotsContainer->SetSlotPadding(FMargin(4.f));
        

        if (!Inventory || !SlotWidgetClass)
                return;

        const TArray<FItemStack>& Slots = Inventory->GetSlots();
        for (int32 i = 0; i < Slots.Num(); ++i)
        {
                UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(this, SlotWidgetClass);
                if (!SlotWidget) continue;

                SlotWidget->SetItemStack(Slots[i]);

                const int32 Row = Columns > 0 ? i / Columns : 0;
                const int32 Col = Columns > 0 ? i % Columns : i;

                UUniformGridSlot* GridSlot = SlotsContainer->AddChildToUniformGrid (SlotWidget, Row, Col);
                if (GridSlot)
                {
                        GridSlot->SetHorizontalAlignment(HAlign_Fill);
                        GridSlot->SetVerticalAlignment(VAlign_Fill);
                }
        }
}

void UInventoryWidget::HandleInventoryComponentUpdated()
{
        IInventoryUpdateInterface::Execute_OnUpdateInventory(this, ObservedInventory.Get());
}

