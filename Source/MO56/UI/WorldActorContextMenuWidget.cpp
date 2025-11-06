// Implementation: Menu entries are generated at runtime and call into player controller or
// skill subsystem callbacks. Use InitializeMenu/InitializeWithActions to populate options
// and listen to OnMenuDismissed to restore AI behaviour when the widget closes.
#include "UI/WorldActorContextMenuWidget.h"

#include "Skills/InspectableComponent.h"
#include "Skills/SkillSystemComponent.h"
#include "Skills/SkillTypes.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SOverlay.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void UWorldActorContextMenuWidget::NativeConstruct()
{
        Super::NativeConstruct();

        bIsFocusable = true; // allow key events
        SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
        SetPositionInViewport(FVector2D(0.f, 0.f), false); // center
        SetFocus(); // try to capture keyboard focus
}

void UWorldActorContextMenuWidget::SetupAsCenteredModal(APlayerController* OwningPC)
{
        ModalOwnerPC = OwningPC;
        ApplyModalInput();
}

void UWorldActorContextMenuWidget::InitializeMenu(UInspectableComponent* Inspectable, USkillSystemComponent* InSkillSystem, const TArray<FSkillInspectionParams>& InParams, TArray<FContextAction>&& AdditionalActions)
{
        InspectableComponent = Inspectable;
        SkillSystem = InSkillSystem;
        BuildEntriesFromInspection(InParams);

        for (FContextAction& Action : AdditionalActions)
        {
                if (!Action.Label.IsEmpty() && Action.Callback)
                {
                        MenuEntries.Add(MoveTemp(Action));
                }
        }

        if (RootWidget.IsValid())
        {
                RebuildWidget();
        }
}

void UWorldActorContextMenuWidget::InitializeWithActions(TArray<FContextAction>&& Actions)
{
        InspectableComponent.Reset();
        SkillSystem.Reset();
        MenuEntries.Reset();

        for (FContextAction& Action : Actions)
        {
                if (!Action.Label.IsEmpty() && Action.Callback)
                {
                        MenuEntries.Add(MoveTemp(Action));
                }
        }

        if (RootWidget.IsValid())
        {
                RebuildWidget();
        }
}

void UWorldActorContextMenuWidget::DismissMenu()
{
        RemoveFromParent();
}

void UWorldActorContextMenuWidget::ApplyModalInput()
{
        if (bModalInputApplied)
        {
                return;
        }

        if (APlayerController* PC = ModalOwnerPC.Get())
        {
                bPrevShowMouseCursor = PC->bShowMouseCursor;
                bPrevEnableClick = PC->bEnableClickEvents;
                bPrevEnableMouseOver = PC->bEnableMouseOverEvents;
                bPrevIgnoreLook = PC->IsLookInputIgnored();
                bPrevIgnoreMove = PC->IsMoveInputIgnored();

                PC->bShowMouseCursor = true;
                PC->bEnableClickEvents = true;
                PC->bEnableMouseOverEvents = true;
                PC->SetIgnoreLookInput(true);
                PC->SetIgnoreMoveInput(true);

                // Focus UI, do not lock, show cursor
                UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, this, EMouseLockMode::DoNotLock, true);

                bModalInputApplied = true;
        }
}

void UWorldActorContextMenuWidget::RestoreInput()
{
        if (!bModalInputApplied)
        {
                return;
        }

        if (APlayerController* PC = ModalOwnerPC.Get())
        {
                PC->SetIgnoreLookInput(bPrevIgnoreLook);
                PC->SetIgnoreMoveInput(bPrevIgnoreMove);
                PC->bShowMouseCursor = bPrevShowMouseCursor;
                PC->bEnableClickEvents = bPrevEnableClick;
                PC->bEnableMouseOverEvents = bPrevEnableMouseOver;

                FInputModeGameOnly GameOnly;
                PC->SetInputMode(GameOnly);
        }

        bModalInputApplied = false;
        ModalOwnerPC.Reset();
}

TSharedRef<SWidget> UWorldActorContextMenuWidget::RebuildWidget()
{
        const FSlateBrush* BackgroundBrush = FAppStyle::Get().GetBrush("Menu.Background");

        TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

        if (MenuEntries.Num() == 0)
        {
                MenuContent->AddSlot()
                        .AutoHeight()
                        .Padding(4.f)
                        [
                                SNew(STextBlock)
                                .Text(NSLOCTEXT("WorldContextMenu", "NoOptions", "Nothing to inspect."))
                        ];
        }
        else
        {
                for (int32 Index = 0; Index < MenuEntries.Num(); ++Index)
                {
                        MenuContent->AddSlot()
                                .AutoHeight()
                                .Padding(4.f)
                                [
                                        SNew(SButton)
                                        .Text(MenuEntries[Index].Label)
                                        .OnClicked(FOnClicked::CreateUObject(this, &UWorldActorContextMenuWidget::HandleEntryClicked, Index))
                                ];
                }
        }

        // Fullscreen overlay: background button catches outside clicks, content is centered on top
        return SAssignNew(RootWidget, SOverlay)
                + SOverlay::Slot()
                        .HAlign(HAlign_Fill)
                        .VAlign(VAlign_Fill)
                        [
                                SNew(SButton)
                                .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
                                .OnClicked(FOnClicked::CreateUObject(this, &UWorldActorContextMenuWidget::HandleBackgroundClicked))
                                [
                                        SNew(SSpacer) // invisible filler
                                        .Size(FVector2D(1.f, 1.f))
                                ]
                        ]
                + SOverlay::Slot()
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        [
                                SNew(SBorder)
                                .BorderImage(BackgroundBrush)
                                .Padding(4.f)
                                [
                                        MenuContent
                                ]
                        ];
}

void UWorldActorContextMenuWidget::ReleaseSlateResources(bool bReleaseChildren)
{
        Super::ReleaseSlateResources(bReleaseChildren);
        RootWidget.Reset();
}

void UWorldActorContextMenuWidget::NativeDestruct()
{
        CloseInternal();
        OnMenuDismissed.Broadcast();
        RestoreInput();
        Super::NativeDestruct();
}

void UWorldActorContextMenuWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
        // Intentionally do nothing so the menu persists until explicit dismissal
        Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UWorldActorContextMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
        const FKey Key = InKeyEvent.GetKey();
        if (Key == EKeys::E || Key == EKeys::Escape)
        {
                DismissMenu();
                return FReply::Handled();
        }
        return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UWorldActorContextMenuWidget::HandleBackgroundClicked()
{
        DismissMenu();
        return FReply::Handled();
}

FReply UWorldActorContextMenuWidget::HandleEntryClicked(int32 OptionIndex)
{
        if (MenuEntries.IsValidIndex(OptionIndex))
        {
                if (MenuEntries[OptionIndex].Callback)
                {
                        MenuEntries[OptionIndex].Callback();
                }

                DismissMenu();
        }

        return FReply::Handled();
}

void UWorldActorContextMenuWidget::CloseInternal()
{
        InspectableComponent.Reset();
        SkillSystem.Reset();
        MenuEntries.Reset();
}

void UWorldActorContextMenuWidget::BuildEntriesFromInspection(const TArray<FSkillInspectionParams>& InParams)
{
        MenuEntries.Reset();

        USkillSystemComponent* System = SkillSystem.Get();
        UInspectableComponent* Inspectable = InspectableComponent.Get();

        for (const FSkillInspectionParams& Params : InParams)
        {
                if (!Params.IsValid())
                {
                        continue;
                }

                FText ButtonLabel = Params.Description;
                if (ButtonLabel.IsEmpty())
                {
                        ButtonLabel = SkillDefinitions::GetKnowledgeDisplayName(Params.KnowledgeId);
                }

                if (ButtonLabel.IsEmpty())
                {
                        ButtonLabel = NSLOCTEXT("WorldContextMenu", "InspectFallback", "Inspect");
                }

                MenuEntries.Emplace(ButtonLabel, [System, Inspectable, Params]()
                {
                        if (System && Inspectable)
                        {
                                System->StartInspectableInspection(Inspectable, Params);
                        }
                });
        }
}
