#include "Menu/MO56MenuPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Menu/MO56MainMenuWidget.h"

AMO56MenuPlayerController::AMO56MenuPlayerController()
{
        bShowMouseCursor = true;
        bEnableClickEvents = true;
        bEnableMouseOverEvents = true;
        MainMenuWidgetClass = UMO56MainMenuWidget::StaticClass();
}

void AMO56MenuPlayerController::BeginPlay()
{
        Super::BeginPlay();

        FInputModeUIOnly InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
        SetShowMouseCursor(true);

        if (!MainMenuWidgetInstance && MainMenuWidgetClass)
        {
                MainMenuWidgetInstance = CreateWidget<UMO56MainMenuWidget>(this, MainMenuWidgetClass);
        }

        if (MainMenuWidgetInstance)
        {
                MainMenuWidgetInstance->AddToViewport();
        }
}

