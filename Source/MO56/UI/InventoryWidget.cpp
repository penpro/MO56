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

void UInventoryWidget::HandlePawnChanged(APawn* /*OldPawn*/, APawn* NewPawn)
{
        BindToInventoryFromPawn(NewPawn);
}

void UInventoryWidget::RefreshInventory(UInventoryComponent* Inventory)
{
        if (SlotsContainer)
        {
                SlotsContainer->ClearChildren();

                if (Inventory && SlotWidgetClass)
                {
                        const TArray<FItemStack>& Slots = Inventory->GetSlots();
                        for (const FItemStack& Stack : Slots)
                        {
                                if (UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(this, SlotWidgetClass))
                                {
                                        SlotWidget->SetItemStack(Stack);
                                        SlotsContainer->AddChild(SlotWidget);
                                }
                        }
                }
        }
}

void UInventoryWidget::HandleInventoryComponentUpdated()
{
        IInventoryUpdateInterface::Execute_OnUpdateInventory(this, ObservedInventory.Get());
}

