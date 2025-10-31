#pragma once

#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UPanelWidget;
class UWidget;

/**
 * HUD widget containing named slots for core gameplay UI elements.
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

private:
        void AddWidgetToContainer(UPanelWidget* Container, UWidget* Widget, bool bClearChildren);
};

