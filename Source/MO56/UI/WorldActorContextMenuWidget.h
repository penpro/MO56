// Implementation: Use this widget for radial/right-click menus in the character HUD. Create
// actions via InitializeMenu/SetCustomActions and bind to player controller functions so
// multiplayer interactions (possess, inspect, inventory) run on the correct authority.
#pragma once

#include "Blueprint/UserWidget.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "Skills/SkillTypes.h"
#include "Delegates/Delegate.h"
#include "Templates/Function.h"
#include "WorldActorContextMenuWidget.generated.h"

class USkillSystemComponent;
class UInspectableComponent;
class APawn;
class APlayerController;

/**
 * Context menu used when right-clicking inspectable world actors.
 *
 * Editor Implementation Guide:
 * 1. Create a UUserWidget Blueprint subclass and add a vertical box or radial layout to host dynamic action buttons.
 * 2. Bind Slate widget references (if using UMG wrappers) to expose RebuildWidget-generated content in Blueprint for styling.
 * 3. When the character opens the menu, call InitializeMenu from Blueprint to populate entries based on inspection data.
 * 4. Subscribe to OnMenuDismissed to close any associated overlays or to resume player input.
 * 5. Implement DismissMenu in Blueprint to animate out the menu before removing it from the viewport.
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

        // Call after AddToViewport when you want modal behavior
        UFUNCTION(BlueprintCallable, Category="UI|ContextMenu")
        void SetupAsCenteredModal(APlayerController* OwningPC);

        void InitializeMenu(UInspectableComponent* Inspectable, USkillSystemComponent* InSkillSystem, const TArray<FSkillInspectionParams>& InParams, TArray<FContextAction>&& AdditionalActions);
        void InitializeWithActions(TArray<FContextAction>&& Actions);
        void DismissMenu();

        DECLARE_MULTICAST_DELEGATE(FOnContextMenuDismissed);
        FOnContextMenuDismissed OnMenuDismissed;

protected:
        virtual void NativeConstruct() override;
        virtual TSharedRef<SWidget> RebuildWidget() override;
        virtual void ReleaseSlateResources(bool bReleaseChildren) override;
        virtual void NativeDestruct() override;

        // Stop auto-dismiss on hover loss
        virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

        // Allow closing with E or Escape
        virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
        // Background click to dismiss
        FReply HandleBackgroundClicked();

        FReply HandleEntryClicked(int32 OptionIndex);
        void CloseInternal();
        void BuildEntriesFromInspection(const TArray<FSkillInspectionParams>& InParams);

        // Modal input bookkeeping
        void ApplyModalInput();
        void RestoreInput();

        TWeakObjectPtr<APlayerController> ModalOwnerPC;
        bool bModalInputApplied = false;
        bool bPrevShowMouseCursor = false;
        bool bPrevEnableClick = false;
        bool bPrevEnableMouseOver = false;
        bool bPrevIgnoreLook = false;
        bool bPrevIgnoreMove = false;

        TWeakObjectPtr<UInspectableComponent> InspectableComponent;
        TWeakObjectPtr<USkillSystemComponent> SkillSystem;
        TArray<FContextAction> MenuEntries;
        TSharedPtr<SWidget> RootWidget;
};
