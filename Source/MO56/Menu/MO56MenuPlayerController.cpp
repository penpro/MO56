#include "Menu/MO56MenuPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Menu/MO56MainMenuWidget.h"

AMO56MenuPlayerController::AMO56MenuPlayerController()
{
        bShowMouseCursor = true;
        bEnableClickEvents = true;
        bEnableMouseOverEvents = true;
}

void AMO56MenuPlayerController::BeginPlay()
{
        Super::BeginPlay();

        if (!MainMenuWidgetClass)
        {
                UE_LOG(LogTemp, Error, TEXT("MainMenuWidgetClass is not set on %s"), *GetName());
                return;
        }

        if (!MainMenuWidgetClass->IsChildOf(UMO56MainMenuWidget::StaticClass()))
        {
                UE_LOG(LogTemp, Error, TEXT("Assigned class %s is not a child of UMO56MainMenuWidget"),
                        *GetNameSafe(*MainMenuWidgetClass));
                return;
        }

        MainMenuWidgetInstance = CreateWidget<UMO56MainMenuWidget>(this, MainMenuWidgetClass);
        if (!MainMenuWidgetInstance)
        {
                UE_LOG(LogTemp, Error, TEXT("CreateWidget failed for %s"), *GetNameSafe(*MainMenuWidgetClass));
                return;
        }

        MainMenuWidgetInstance->AddToViewport(10);

        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(MainMenuWidgetInstance->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
        bShowMouseCursor = true;

        UE_LOG(LogTemp, Log, TEXT("Menu widget shown: %s (class: %s)"),
                *GetNameSafe(MainMenuWidgetInstance),
                *GetNameSafe(*MainMenuWidgetClass));
}

