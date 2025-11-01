#include "UI/InventorySlotWidget.h"

#include "UI/InventorySlotDragOperation.h"
#include "UI/InventorySlotDragVisual.h"
#include "UI/InventorySlotMenuWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "InventoryComponent.h"
#include "InventoryContainer.h"
#include "MO56PlayerController.h"
#include "ItemData.h"
#include "Skills/SkillSystemComponent.h"
#include "InputCoreTypes.h"
#include "Math/UnrealMathUtility.h"
#include "UObject/Class.h"

namespace UE::InventorySlotWidget::Private
{
        static void RequestInventoryOwnership(UInventorySlotWidget* SlotWidget, UInventoryComponent* Inventory)
        {
                if (!SlotWidget || !Inventory)
                {
                        return;
                }

                AActor* InventoryOwner = Inventory->GetOwner();
                if (!InventoryOwner)
                {
                        return;
                }

                if (AInventoryContainer* ContainerActor = Cast<AInventoryContainer>(InventoryOwner))
                {
                        if (APlayerController* OwningPlayer = SlotWidget->GetOwningPlayer())
                        {
                                if (ContainerActor->GetOwner() == OwningPlayer)
                                {
                                        return;
                                }

                                if (AMO56PlayerController* MOController = Cast<AMO56PlayerController>(OwningPlayer))
                                {
                                        MOController->RequestContainerInventoryOwnership(ContainerActor);
                                }
                        }
                }
        }

        static bool InvokeInventoryBoolFunction(UInventorySlotWidget* SlotWidget, UInventoryComponent* Inventory, int32 SlotIndex, const FName& FunctionName)
        {
                if (!Inventory || SlotIndex == INDEX_NONE)
                {
                        return false;
                }

                if (SlotWidget)
                {
                        RequestInventoryOwnership(SlotWidget, Inventory);
                }

                if (UFunction* Function = Inventory->FindFunction(FunctionName))
                {
                        struct FInventorySlotFunctionParams
                        {
                                int32 SlotIndex = INDEX_NONE;
                                bool ReturnValue = false;
                        };

                        FInventorySlotFunctionParams Params;
                        Params.SlotIndex = SlotIndex;

                        Inventory->ProcessEvent(Function, &Params);
                        return Params.ReturnValue;
                }

                return false;
        }

        static bool InvokeInventoryTransferFunction(UInventorySlotWidget* SlotWidget, UInventoryComponent* Inventory, int32 SourceSlotIndex, int32 TargetSlotIndex, const FName& FunctionName)
        {
                if (!Inventory)
                {
                        return false;
                }

                if (SourceSlotIndex == INDEX_NONE || TargetSlotIndex == INDEX_NONE)
                {
                        return false;
                }

                if (SlotWidget)
                {
                        RequestInventoryOwnership(SlotWidget, Inventory);
                }

                if (UFunction* Function = Inventory->FindFunction(FunctionName))
                {
                        struct FInventoryTransferFunctionParams
                        {
                                int32 SourceSlotIndex = INDEX_NONE;
                                int32 TargetSlotIndex = INDEX_NONE;
                                bool ReturnValue = false;
                        };

                        FInventoryTransferFunctionParams Params;
                        Params.SourceSlotIndex = SourceSlotIndex;
                        Params.TargetSlotIndex = TargetSlotIndex;

                        Inventory->ProcessEvent(Function, &Params);
                        return Params.ReturnValue;
                }

                return false;
        }
}

UInventorySlotWidget::UInventorySlotWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ContextMenuClass = UInventorySlotMenuWidget::StaticClass();
	DragVisualClass = UInventorySlotDragVisual::StaticClass();
}

void UInventorySlotWidget::SetItemStack(const FItemStack& Stack)
{
        CachedStack = Stack;

        if (ItemIcon)
        {
                if (Stack.Item)
                {
                        if (UTexture2D* IconTexture = Stack.Item->Icon.LoadSynchronous())
                        {
                                ItemIcon->SetBrushFromTexture(IconTexture);
                                ItemIcon->SetVisibility(ESlateVisibility::Visible);
                        }
                        else
                        {
                                ItemIcon->SetBrushFromTexture(nullptr);
                                ItemIcon->SetVisibility(ESlateVisibility::Hidden);
                        }
                }
                else
                {
                        ItemIcon->SetBrushFromTexture(nullptr);
                        ItemIcon->SetVisibility(ESlateVisibility::Hidden);
                }
        }

        if (QuantityText)
        {
                if (Stack.Quantity > 1)
                {
                        QuantityText->SetText(FText::AsNumber(Stack.Quantity));
                        QuantityText->SetVisibility(ESlateVisibility::Visible);
                }
                else if (Stack.Item)
                {
                        QuantityText->SetText(FText::GetEmpty());
                        QuantityText->SetVisibility(ESlateVisibility::Hidden);
                }
                else
                {
                        QuantityText->SetText(FText::GetEmpty());
                        QuantityText->SetVisibility(ESlateVisibility::Hidden);
                }

                if (QuantityBadge)
                {
                        const bool bShowBadge = (Stack.Item != nullptr) && (Stack.Quantity > 1);
                        QuantityBadge->SetVisibility(bShowBadge ? ESlateVisibility::Visible
                                                                : ESlateVisibility::Collapsed);
                }
                else if (QuantityText)
                {
                        // Fallback if the SizeBox wasn’t bound
                        const bool bShow = (Stack.Item != nullptr) && (Stack.Quantity > 1);
                        QuantityText->SetVisibility(bShow ? ESlateVisibility::Visible
                                                          : ESlateVisibility::Collapsed);
                }
        }

        if (CachedStack.IsEmpty())
        {
                CloseContextMenu();
        }
}

void UInventorySlotWidget::InitializeSlot(UInventoryComponent* Inventory, int32 InSlotIndex)
{
        ObservedInventory = Inventory;
        SlotIndex = InSlotIndex;
}

bool UInventorySlotWidget::CanSplitStack() const
{
        return CachedStack.Item != nullptr && CachedStack.Quantity > 1;
}

bool UInventorySlotWidget::HasItem() const
{
        return CachedStack.Item != nullptr && CachedStack.Quantity > 0;
}

bool UInventorySlotWidget::HandleSplitStack()
{
        static const FName SplitStackAtIndexName(TEXT("SplitStackAtIndex"));
        return UE::InventorySlotWidget::Private::InvokeInventoryBoolFunction(this, ObservedInventory.Get(), SlotIndex, SplitStackAtIndexName);
}

bool UInventorySlotWidget::HandleDestroyItem()
{
        static const FName DestroyItemAtIndexName(TEXT("DestroyItemAtIndex"));
        return UE::InventorySlotWidget::Private::InvokeInventoryBoolFunction(this, ObservedInventory.Get(), SlotIndex, DestroyItemAtIndexName);
}

bool UInventorySlotWidget::HandleDropAllItems()
{
        static const FName DropItemAtIndexName(TEXT("DropItemAtIndex"));
        return UE::InventorySlotWidget::Private::InvokeInventoryBoolFunction(this, ObservedInventory.Get(), SlotIndex, DropItemAtIndexName);
}

bool UInventorySlotWidget::HandleDropOneItem()
{
        static const FName DropSingleItemAtIndexName(TEXT("DropSingleItemAtIndex"));
        return UE::InventorySlotWidget::Private::InvokeInventoryBoolFunction(this, ObservedInventory.Get(), SlotIndex, DropSingleItemAtIndexName);
}

void UInventorySlotWidget::CloseContextMenu()
{
        if (UInventorySlotMenuWidget* Menu = ActiveContextMenu.Get())
        {
                Menu->DismissMenu();
        }
}

void UInventorySlotWidget::NotifyContextMenuClosed(UInventorySlotMenuWidget* ClosedMenu)
{
        if (ActiveContextMenu.Get() == ClosedMenu)
        {
                ActiveContextMenu = nullptr;
        }
}

void UInventorySlotWidget::SetSkillSystem(USkillSystemComponent* InSkillSystem)
{
        SkillSystem = InSkillSystem;
}

bool UInventorySlotWidget::HandleInspectItem()
{
        if (!CanInspectItem())
        {
                return false;
        }

        if (USkillSystemComponent* System = SkillSystem.Get())
        {
                if (System->StartItemInspection(CachedStack.Item, this))
                {
                        return true;
                }
        }

        return false;
}

bool UInventorySlotWidget::CanInspectItem() const
{
        if (!CachedStack.Item)
        {
                return false;
        }

        if (!SkillSystem.IsValid())
        {
                return false;
        }

        return !CachedStack.Item->KnowledgeTag.IsNone();
}

void UInventorySlotWidget::NativeDestruct()
{
        CloseContextMenu();
        Super::NativeDestruct();
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
        if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
        {
                ShowContextMenu(InMouseEvent.GetScreenSpacePosition());
                return FReply::Handled();
        }

        if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
        {
                CloseContextMenu();

                if (HasItem())
                {
                        return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
                }
        }

        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
        Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

        UInventoryComponent* Inventory = ObservedInventory.Get();
        if (!Inventory || SlotIndex == INDEX_NONE || !HasItem())
        {
                return;
        }

        CloseContextMenu();

	UInventorySlotDragOperation* DragOperation = NewObject<UInventorySlotDragOperation>(this);
	if (!DragOperation)
	{
		return;
	}

	DragOperation->InitializeOperation(Inventory, SlotIndex, CachedStack);
	DragOperation->Pivot = EDragPivot::MouseDown;

	if (DragVisualClass)
	{
		UInventorySlotDragVisual* DragVisual = nullptr;

		if (APlayerController* OwningPlayer = GetOwningPlayer())
		{
			DragVisual = CreateWidget<UInventorySlotDragVisual>(OwningPlayer, DragVisualClass);
		}
		else if (UWorld* World = GetWorld())
		{
			DragVisual = CreateWidget<UInventorySlotDragVisual>(World, DragVisualClass);
		}

		if (DragVisual)
		{
			DragVisual->SetDraggedStack(CachedStack);
			DragOperation->DefaultDragVisual = DragVisual;
		}
	}

	OutOperation = DragOperation;
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
        if (UInventorySlotDragOperation* DragOperation = Cast<UInventorySlotDragOperation>(InOperation))
        {
                UInventoryComponent* TargetInventory = ObservedInventory.Get();
                UInventoryComponent* SourceInventory = DragOperation->GetSourceInventory();

                if (TargetInventory && SourceInventory && SlotIndex != INDEX_NONE)
                {
                        const int32 SourceIndex = DragOperation->GetSourceSlotIndex();
                        if (SourceIndex != INDEX_NONE)
                        {
                                if (TargetInventory == SourceInventory)
                                {
                                        if (SourceIndex != SlotIndex)
                                        {
                                                static const FName TransferFunctionName(TEXT("TransferItemBetweenSlots"));
                                                if (UE::InventorySlotWidget::Private::InvokeInventoryTransferFunction(this, TargetInventory, SourceIndex, SlotIndex, TransferFunctionName))
                                                {
                                                        CloseContextMenu();
                                                        return true;
                                                }
                                        }
                                }
                                else
                                {
                                        UE::InventorySlotWidget::Private::RequestInventoryOwnership(this, SourceInventory);
                                        UE::InventorySlotWidget::Private::RequestInventoryOwnership(this, TargetInventory);
                                        if (SourceInventory->TransferItemToInventory(TargetInventory, SourceIndex, SlotIndex))
                                        {
                                                CloseContextMenu();
                                                return true;
                                        }
                                }
                        }
                }
}

        return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UInventorySlotWidget::ShowContextMenu(const FVector2D& ScreenPosition)
{
        if (!HasItem())
        {
                CloseContextMenu();
                return;
        }

        if (!ContextMenuClass)
        {
                return;
        }

        if (UInventorySlotMenuWidget* ExistingMenu = ActiveContextMenu.Get())
        {
                ExistingMenu->DismissMenu();
        }

        UWorld* World = GetWorld();
        if (!World)
        {
                return;
        }

        APlayerController* OwningPlayer = GetOwningPlayer();

        UInventorySlotMenuWidget* Menu = nullptr;
        if (OwningPlayer)
        {
                Menu = CreateWidget<UInventorySlotMenuWidget>(OwningPlayer, ContextMenuClass);
        }
        else
        {
                Menu = CreateWidget<UInventorySlotMenuWidget>(World, ContextMenuClass);
        }
        if (!Menu)
        {
                return;
        }

        Menu->SetOwningSlot(this);
        Menu->AddToViewport(1000);

        FVector2D ViewportPosition = ScreenPosition;
        bool bHasViewportPosition = false;

        if (OwningPlayer)
        {
                float MouseX = 0.f;
                float MouseY = 0.f;
                if (UWidgetLayoutLibrary::GetMousePositionScaledByDPI(OwningPlayer, MouseX, MouseY))
                {
                        ViewportPosition = FVector2D(MouseX, MouseY);
                        bHasViewportPosition = true;
                }
                else
                {
                        const FVector2D MouseViewportPos = UWidgetLayoutLibrary::GetMousePositionOnViewport(OwningPlayer);
                        if (!MouseViewportPos.ContainsNaN())
                        {
                                ViewportPosition = MouseViewportPos;
                                bHasViewportPosition = true;
                        }
                }
        }

        if (!bHasViewportPosition)
        {
                float ViewportScale = 1.f;
                if (OwningPlayer)
                {
                        ViewportScale = UWidgetLayoutLibrary::GetViewportScale(OwningPlayer);
                }
                else if (APlayerController* PC = World->GetFirstPlayerController())
                {
                        ViewportScale = UWidgetLayoutLibrary::GetViewportScale(PC);
                }

                if (ViewportScale > KINDA_SMALL_NUMBER)
                {
                        ViewportPosition = ScreenPosition / ViewportScale;
                }
        }

        Menu->SetAlignmentInViewport(FVector2D::ZeroVector);
        Menu->SetPositionInViewport(ViewportPosition, false);
        ActiveContextMenu = Menu;
}

