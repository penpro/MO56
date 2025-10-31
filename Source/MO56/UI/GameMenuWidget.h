#pragma once

#include "Blueprint/UserWidget.h"
#include "GameMenuWidget.generated.h"

class UButton;
class UMO56SaveSubsystem;

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
        TObjectPtr<UButton> ExitGameButton;

        UFUNCTION()
        void HandleNewGameClicked();

        UFUNCTION()
        void HandleLoadGameClicked();

        UFUNCTION()
        void HandleExitGameClicked();

private:
        void EnsureSaveSubsystem();

        TWeakObjectPtr<UMO56SaveSubsystem> CachedSaveSubsystem;
};

