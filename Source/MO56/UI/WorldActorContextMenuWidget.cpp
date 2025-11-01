// Implementation: Menu entries are generated at runtime and call into player controller or
// skill subsystem callbacks. Use InitializeMenu/InitializeWithActions to populate options
// and listen to OnMenuDismissed to restore AI behaviour when the widget closes.
#include "UI/WorldActorContextMenuWidget.h"

#include "Skills/InspectableComponent.h"
#include "Skills/SkillSystemComponent.h"
#include "Skills/SkillTypes.h"
#include "Input/Events.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

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

        return SAssignNew(RootWidget, SBorder)
                .BorderImage(BackgroundBrush)
                .Padding(4.f)
                [
                        MenuContent
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
        Super::NativeDestruct();
}

void UWorldActorContextMenuWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
        Super::NativeOnMouseLeave(InMouseEvent);
        DismissMenu();
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
