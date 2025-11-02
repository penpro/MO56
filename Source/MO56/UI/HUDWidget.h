#pragma once

#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UPanelWidget;
class UWidget;
class UCharacterSkillMenu;
class UInspectionCountdownWidget;
class USkillSystemComponent;

/**
 * HUD widget containing named slots for core gameplay UI elements.
 *
 * Editor Implementation Guide:
 * 1. Create a Blueprint inheriting from UHUDWidget and bind panel widgets (CharacterStatusContainer, etc.) via named slots.
 * 2. Add overlay widgets in the designer to match the containers defined here for inventory, skills, menus, and crosshair.
 * 3. In your PlayerController Blueprint, create the HUD widget on BeginPlay and add it to the viewport.
 * 4. Call ConfigureSkillMenu/ConfigureInspectionCountdown to register supporting widget classes before toggling them.
 * 5. Use AddCharacterStatusWidget/AddLeftInventoryWidget from Blueprint to attach runtime-created panels into the HUD.
 */
UCLASS()
class MO56_API UHUDWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        /** Container for character status UI elements (health, stamina, etc.) */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> CharacterStatusContainer;

        /** Container for character skill UI elements */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> CharacterSkillContainer;

        /** Container for the crosshair UI element */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> CrosshairContainer;

        /** Container for the left inventory panel */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> LeftInventoryContainer;

        /** Container for the right inventory panel */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> RightInventoryContainer;

        /** Container for the mini map widget */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> MiniMapContainer;

        /** Container for the game menu widget */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> GameMenuContainer;

public:
        /** Adds the given widget to the character status container, if available */
        UFUNCTION(BlueprintCallable, Category = "HUD")
        void AddCharacterStatusWidget(UWidget* Widget);

        /** Sets the crosshair widget, replacing any existing child */
        UFUNCTION(BlueprintCallable, Category = "HUD")
        void SetCrosshairWidget(UWidget* Widget);

        /** Adds a widget to the left inventory container */
        UFUNCTION(BlueprintCallable, Category = "HUD")
        void AddLeftInventoryWidget(UWidget* Widget);

        /** Adds a widget to the right inventory container */
        UFUNCTION(BlueprintCallable, Category = "HUD")
        void AddRightInventoryWidget(UWidget* Widget);

        /** Sets the mini map widget */
        UFUNCTION(BlueprintCallable, Category = "HUD")
        void SetMiniMapWidget(UWidget* Widget);

        /** Sets the game menu widget */
        UFUNCTION(BlueprintCallable, Category = "HUD")
        void SetGameMenuWidget(UWidget* Widget);

        /** Sets the character skill widget. */
        UFUNCTION(BlueprintCallable, Category = "HUD")
        void SetCharacterSkillWidget(UWidget* Widget);

        /** Configures the skill menu that should be lazily created for the HUD. */
        void ConfigureSkillMenu(TSubclassOf<UCharacterSkillMenu> InMenuClass, USkillSystemComponent* InSkillSystem);

        /** Ensures that the skill menu instance exists, creating it if necessary. */
        UCharacterSkillMenu* EnsureSkillMenuInstance();

        /** Toggles the visibility of the skill menu, returning the new visibility state. */
        bool ToggleSkillMenu();

        /** Explicitly sets the skill menu visibility. */
        void SetSkillMenuVisibility(bool bVisible);

        /** Returns true if the skill menu is currently visible. */
        bool IsSkillMenuVisible() const;

        /** Returns the underlying skill menu instance, if it exists. */
        UCharacterSkillMenu* GetSkillMenuInstance() const { return SkillMenuInstance.Get(); }

        /** Registers the inspection countdown class that should be displayed during timers. */
        void ConfigureInspectionCountdown(TSubclassOf<UInspectionCountdownWidget> InCountdownClass);

        /** Ensures the countdown overlay is instantiated. */
        UInspectionCountdownWidget* EnsureCountdownOverlay();

        /** Shows the inspection countdown overlay. */
        void ShowInspection(const FText& Description, float DurationSeconds);

        /** Updates the inspection countdown overlay. */
        void UpdateInspection(float ElapsedSeconds, float DurationSeconds);

        /** Hides the inspection countdown overlay. */
        void HideInspection();

protected:
        virtual void NativeDestruct() override;

private:
        void AddWidgetToContainer(UPanelWidget* Container, UWidget* Widget, bool bClearChildren);

        void SetSkillSystemComponent(USkillSystemComponent* InSkillSystem);
        void BindSkillDelegates(USkillSystemComponent* InSkillSystem);
        void UnbindSkillDelegates(USkillSystemComponent* InSkillSystem);

        UFUNCTION()
        void HandleInspectionStarted(const FText& Description, float Duration);

        UFUNCTION()
        void HandleInspectionProgress(float Elapsed, float Duration);

        UFUNCTION()
        void HandleInspectionCompleted();

        UFUNCTION()
        void HandleInspectionCancelled(FName Reason);

        UFUNCTION()
        void HandleCountdownCancelRequested();

        void UpdateCountdownVisibility(bool bVisible) const;
        void HideCountdownInternal();

private:
        UPROPERTY(EditAnywhere, Category = "Skills")
        TSubclassOf<UCharacterSkillMenu> SkillMenuClass;

        UPROPERTY(Transient)
        TObjectPtr<UCharacterSkillMenu> SkillMenuInstance;

        UPROPERTY(Transient)
        TWeakObjectPtr<USkillSystemComponent> SkillSystemRef;

        UPROPERTY(EditAnywhere, Category = "Inspection")
        TSubclassOf<UInspectionCountdownWidget> InspectionCountdownClass;

        UPROPERTY(Transient)
        TObjectPtr<UInspectionCountdownWidget> InspectionCountdownInstance;
};

