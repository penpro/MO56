#include "UI/InventoryWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Internationalization/NumberFormattingSettings.h"
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
        UpdateInventoryStats(Inventory);
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

                SlotWidget->InitializeSlot(Inventory, i);
                SlotWidget->SetItemStack(Slots[i]);

                const int32 Row = Columns > 0 ? i / Columns : 0;
                const int32 Col = Columns > 0 ? i % Columns : i;

                UUniformGridSlot* GridSlot = SlotsContainer->AddChildToUniformGrid (SlotWidget, Row, Col);
                if (GridSlot)
                {
                        GridSlot->SetHorizontalAlignment(HAlign_Center);
                        GridSlot->SetVerticalAlignment(VAlign_Center);
                }
        }
}

void UInventoryWidget::HandleInventoryComponentUpdated()
{
        IInventoryUpdateInterface::Execute_OnUpdateInventory(this, ObservedInventory.Get());
}

void UInventoryWidget::UpdateInventoryStats(UInventoryComponent* Inventory)
{
        FNumberFormattingOptions NumberOptions;
        NumberOptions.MaximumFractionalDigits = 2;
        NumberOptions.MinimumFractionalDigits = 0;

        const float TotalWeightValue = Inventory ? Inventory->GetTotalWeight() : 0.f;
        const float TotalVolumeValue = Inventory ? Inventory->GetTotalVolume() : 0.f;
        const float MaxWeightValue = Inventory ? Inventory->MaxWeight : 0.f;
        const float MaxVolumeValue = Inventory ? Inventory->MaxVolume : 0.f;

        if (WeightTotal)
        {
                WeightTotal->SetText(FText::AsNumber(TotalWeightValue, &NumberOptions));
        }

        if (VolumeTotal)
        {
                VolumeTotal->SetText(FText::AsNumber(TotalVolumeValue, &NumberOptions));
        }

        if (MaxWeight)
        {
                MaxWeight->SetText(FText::AsNumber(MaxWeightValue, &NumberOptions));
        }

        if (MaxVolume)
        {
                MaxVolume->SetText(FText::AsNumber(MaxVolumeValue, &NumberOptions));
        }
}

