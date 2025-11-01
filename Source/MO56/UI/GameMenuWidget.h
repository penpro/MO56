// Implementation: Create a UMG widget inheriting from this class, bind the buttons named
// NewGameButton/LoadGameButton/CreateNewSaveButton/SaveGameButton/SaveAndExitButton/ExitGameButton, and place a
// panel named FocusContainer for nested menus. Assign the widget class to the character HUD
// so menu actions forward to the player controller and save subsystem.
#pragma once

#include "Blueprint/UserWidget.h"
#include "GameMenuWidget.generated.h"

class UButton;
class UPanelWidget;
class UUserWidget;
class USaveGameMenuWidget;
class UMO56SaveSubsystem;
class AMO56PlayerController;

/**
 * Simple in-game menu used to trigger save/load operations and exit the game.
 */
UCLASS()
class MO56_API UGameMenuWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        virtual void NativeOnInitialized() override;
        virtual void NativeConstruct() override;

        /** Assigns the save subsystem that handles persistence. */
        UFUNCTION(BlueprintCallable, Category = "Menu")
        void SetSaveSubsystem(UMO56SaveSubsystem* Subsystem);

protected:
        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UButton> NewGameButton;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UButton> LoadGameButton;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UButton> CreateNewSaveButton;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UButton> SaveGameButton;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UButton> SaveAndExitButton;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UButton> ExitGameButton;

        /** Container that hosts focusable child widgets such as the save menu. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> FocusContainer;

        /** Class used when spawning the save-game menu. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
        TSubclassOf<USaveGameMenuWidget> SaveGameMenuClass;

        UFUNCTION()
        void HandleNewGameClicked();

        UFUNCTION()
        void HandleLoadGameClicked();

        UFUNCTION()
        void HandleCreateNewSaveClicked();

        UFUNCTION()
        void HandleSaveGameClicked();

        UFUNCTION()
        void HandleSaveAndExitClicked();

        UFUNCTION()
        void HandleExitGameClicked();

private:
        void EnsureSaveSubsystem();
        void ShowFocusWidget(UUserWidget* Widget);
        void ClearFocusWidget();

        AMO56PlayerController* ResolvePlayerController() const;

        UFUNCTION()
        void HandleSaveGameLoaded();

        TWeakObjectPtr<UMO56SaveSubsystem> CachedSaveSubsystem;
        UPROPERTY(Transient)
        TObjectPtr<USaveGameMenuWidget> SaveGameMenuInstance;
};

