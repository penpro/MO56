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

void UWorldActorContextMenuWidget::InitializeMenu(UInspectableComponent* Inspectable, USkillSystemComponent* InSkillSystem, const TArray<FSkillInspectionParams>& InParams)
{
        InspectableComponent = Inspectable;
        SkillSystem = InSkillSystem;
        InspectionOptions.Reset();

        for (const FSkillInspectionParams& Params : InParams)
        {
                if (Params.IsValid())
                {
                        InspectionOptions.Add(Params);
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

        if (InspectionOptions.Num() == 0)
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
                for (int32 Index = 0; Index < InspectionOptions.Num(); ++Index)
                {
                        const FSkillInspectionParams& Params = InspectionOptions[Index];
                        FText ButtonLabel = Params.Description;
                        if (ButtonLabel.IsEmpty())
                        {
                                ButtonLabel = SkillDefinitions::GetKnowledgeDisplayName(Params.KnowledgeId);
                        }

                        if (ButtonLabel.IsEmpty())
                        {
                                ButtonLabel = NSLOCTEXT("WorldContextMenu", "InspectFallback", "Inspect");
                        }

                        MenuContent->AddSlot()
                                .AutoHeight()
                                .Padding(4.f)
                                [
                                        SNew(SButton)
                                        .Text(ButtonLabel)
                                        .OnClicked(FOnClicked::CreateUObject(this, &UWorldActorContextMenuWidget::HandleInspectOptionClicked, Index))
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
        Super::NativeDestruct();
}

void UWorldActorContextMenuWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
        Super::NativeOnMouseLeave(InMouseEvent);
        DismissMenu();
}

FReply UWorldActorContextMenuWidget::HandleInspectOptionClicked(int32 OptionIndex)
{
        USkillSystemComponent* System = SkillSystem.Get();
        UInspectableComponent* Inspectable = InspectableComponent.Get();

        if (System && Inspectable && InspectionOptions.IsValidIndex(OptionIndex))
        {
                if (System->StartInspectableInspection(Inspectable, InspectionOptions[OptionIndex]))
                {
                        DismissMenu();
                }
        }

        return FReply::Handled();
}

void UWorldActorContextMenuWidget::CloseInternal()
{
        InspectableComponent.Reset();
        SkillSystem.Reset();
        InspectionOptions.Reset();
}
