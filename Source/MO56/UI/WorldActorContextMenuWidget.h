#pragma once

#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "Skills/SkillTypes.h"
#include "WorldActorContextMenuWidget.generated.h"

class USkillSystemComponent;
class UInspectableComponent;

/**
 * Context menu used when right-clicking inspectable world actors.
 */
UCLASS()
class MO56_API UWorldActorContextMenuWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        void InitializeMenu(UInspectableComponent* Inspectable, USkillSystemComponent* InSkillSystem, const TArray<FSkillInspectionParams>& InParams);
        void DismissMenu();

protected:
        virtual TSharedRef<SWidget> RebuildWidget() override;
        virtual void ReleaseSlateResources(bool bReleaseChildren) override;
        virtual void NativeDestruct() override;
        virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
        FReply HandleInspectOptionClicked(int32 OptionIndex);

        void CloseInternal();

        TWeakObjectPtr<UInspectableComponent> InspectableComponent;
        TWeakObjectPtr<USkillSystemComponent> SkillSystem;
        TArray<FSkillInspectionParams> InspectionOptions;
        TSharedPtr<SWidget> RootWidget;
};
