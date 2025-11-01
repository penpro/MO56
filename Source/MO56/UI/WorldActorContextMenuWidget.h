// Implementation: Use this widget for radial/right-click menus in the character HUD. Create
// actions via InitializeMenu/SetCustomActions and bind to player controller functions so
// multiplayer interactions (possess, inspect, inventory) run on the correct authority.
#pragma once

#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "Skills/SkillTypes.h"
#include "Delegates/Delegate.h"
#include "Templates/Function.h"
#include "WorldActorContextMenuWidget.generated.h"

class USkillSystemComponent;
class UInspectableComponent;
class APawn;

/**
 * Context menu used when right-clicking inspectable world actors.
 */
UCLASS()
class MO56_API UWorldActorContextMenuWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        struct FContextAction
        {
                FText Label;
                TFunction<void()> Callback;

                FContextAction() = default;

                FContextAction(const FText& InLabel, TFunction<void()> InCallback)
                        : Label(InLabel)
                        , Callback(MoveTemp(InCallback))
                {
                }
        };

        void InitializeMenu(UInspectableComponent* Inspectable, USkillSystemComponent* InSkillSystem, const TArray<FSkillInspectionParams>& InParams, TArray<FContextAction>&& AdditionalActions);
        void InitializeWithActions(TArray<FContextAction>&& Actions);
        void DismissMenu();

        DECLARE_MULTICAST_DELEGATE(FOnContextMenuDismissed);
        FOnContextMenuDismissed OnMenuDismissed;

protected:
        virtual TSharedRef<SWidget> RebuildWidget() override;
        virtual void ReleaseSlateResources(bool bReleaseChildren) override;
        virtual void NativeDestruct() override;
        virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
        FReply HandleEntryClicked(int32 OptionIndex);

        void CloseInternal();
        void BuildEntriesFromInspection(const TArray<FSkillInspectionParams>& InParams);

        TWeakObjectPtr<UInspectableComponent> InspectableComponent;
        TWeakObjectPtr<USkillSystemComponent> SkillSystem;
        TArray<FContextAction> MenuEntries;
        TSharedPtr<SWidget> RootWidget;
};
