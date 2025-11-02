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

        if (!MainMenuWidgetInstance && MainMenuWidgetClass)
        {
                MainMenuWidgetInstance = CreateWidget<UMO56MainMenuWidget>(this, MainMenuWidgetClass);
                if (MainMenuWidgetInstance)
                {
                        // If you added EnsureBuilt(), call it before AddToViewport for pure C++ layout
                        // MainMenuWidgetInstance->EnsureBuilt();

                        MainMenuWidgetInstance->AddToViewport();

                        FInputModeUIOnly InputMode;
                        InputMode.SetWidgetToFocus(MainMenuWidgetInstance->TakeWidget());
                        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                        SetInputMode(InputMode);

                        bShowMouseCursor = true;

                        
                }
        }
}

