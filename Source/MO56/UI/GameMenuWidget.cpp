// Implementation: Bind this widget to the character HUD, set the button class names in UMG,
// and ensure the owning player controller is AMO56PlayerController so game flow requests
// are executed on the server.
#include "UI/GameMenuWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Save/MO56SaveSubsystem.h"
#include "UI/SaveGameMenuWidget.h"
#include "MO56PlayerController.h"

void UGameMenuWidget::NativeOnInitialized()
{
        Super::NativeOnInitialized();

        if (NewGameButton)
        {
                NewGameButton->OnClicked.AddDynamic(this, &UGameMenuWidget::HandleNewGameClicked);
        }

        if (LoadGameButton)
        {
                LoadGameButton->OnClicked.AddDynamic(this, &UGameMenuWidget::HandleLoadGameClicked);
        }

        if (SaveGameButton)
        {
                SaveGameButton->OnClicked.AddDynamic(this, &UGameMenuWidget::HandleSaveGameClicked);
        }

        if (SaveAndExitButton)
        {
                SaveAndExitButton->OnClicked.AddDynamic(this, &UGameMenuWidget::HandleSaveAndExitClicked);
        }

        if (ExitGameButton)
        {
                ExitGameButton->OnClicked.AddDynamic(this, &UGameMenuWidget::HandleExitGameClicked);
        }
}

void UGameMenuWidget::NativeConstruct()
{
        Super::NativeConstruct();
        EnsureSaveSubsystem();

        if (FocusContainer)
        {
                FocusContainer->SetVisibility(ESlateVisibility::Collapsed);
        }
}

void UGameMenuWidget::SetSaveSubsystem(UMO56SaveSubsystem* Subsystem)
{
        CachedSaveSubsystem = Subsystem;

        if (SaveGameMenuInstance)
        {
                SaveGameMenuInstance->SetSaveSubsystem(Subsystem);
        }
}

void UGameMenuWidget::HandleNewGameClicked()
{
        ClearFocusWidget();
        if (AMO56PlayerController* PC = ResolvePlayerController())
        {
                PC->RequestNewGame();
        }
}

void UGameMenuWidget::HandleLoadGameClicked()
{
        EnsureSaveSubsystem();
        if (UMO56SaveSubsystem* SaveSubsystem = CachedSaveSubsystem.Get())
        {
                if (!FocusContainer || !SaveGameMenuClass)
                {
                        SaveSubsystem->LoadGame();
                        return;
                }

                if (!SaveGameMenuInstance)
                {
                        if (APlayerController* PC = GetOwningPlayer())
                        {
                                SaveGameMenuInstance = CreateWidget<USaveGameMenuWidget>(PC, SaveGameMenuClass);
                        }
                        else
                        {
                                SaveGameMenuInstance = CreateWidget<USaveGameMenuWidget>(GetWorld(), SaveGameMenuClass);
                        }

                        if (SaveGameMenuInstance)
                        {
                                SaveGameMenuInstance->OnSaveLoaded.AddDynamic(this, &UGameMenuWidget::HandleSaveGameLoaded);
                        }
                }

                if (SaveGameMenuInstance)
                {
                        SaveGameMenuInstance->SetSaveSubsystem(SaveSubsystem);
                        SaveGameMenuInstance->RefreshSaveEntries();
                        ShowFocusWidget(SaveGameMenuInstance);
                }
        }
}

void UGameMenuWidget::HandleSaveGameClicked()
{
        ClearFocusWidget();
        if (AMO56PlayerController* PC = ResolvePlayerController())
        {
                PC->RequestSaveGame();
        }
        else if (UMO56SaveSubsystem* SaveSubsystem = CachedSaveSubsystem.Get())
        {
                SaveSubsystem->SaveGame();
        }
}

void UGameMenuWidget::HandleSaveAndExitClicked()
{
        ClearFocusWidget();
        if (AMO56PlayerController* PC = ResolvePlayerController())
        {
                PC->RequestSaveAndExit();
        }
        else
        {
                EnsureSaveSubsystem();
                if (UMO56SaveSubsystem* SaveSubsystem = CachedSaveSubsystem.Get())
                {
                        SaveSubsystem->SaveGame();
                }

                HandleExitGameClicked();
        }
}

void UGameMenuWidget::HandleExitGameClicked()
{
        ClearFocusWidget();
        APlayerController* OwningController = GetOwningPlayer();
        UKismetSystemLibrary::QuitGame(this, OwningController, EQuitPreference::Quit, false);
}

void UGameMenuWidget::EnsureSaveSubsystem()
{
        if (CachedSaveSubsystem.IsValid())
        {
                return;
        }

        if (UGameInstance* GameInstance = GetGameInstance())
        {
                CachedSaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>();
        }
}

void UGameMenuWidget::ShowFocusWidget(UUserWidget* Widget)
{
        if (!FocusContainer || !Widget)
        {
                return;
        }

        if (Widget->GetParent() != FocusContainer)
        {
                FocusContainer->ClearChildren();
                FocusContainer->AddChild(Widget);
        }

        Widget->SetVisibility(ESlateVisibility::Visible);
        FocusContainer->SetVisibility(ESlateVisibility::Visible);
}

void UGameMenuWidget::ClearFocusWidget()
{
        if (FocusContainer)
        {
                FocusContainer->ClearChildren();
                FocusContainer->SetVisibility(ESlateVisibility::Collapsed);
        }
}

void UGameMenuWidget::HandleSaveGameLoaded()
{
        ClearFocusWidget();
        SetVisibility(ESlateVisibility::Collapsed);
}

AMO56PlayerController* UGameMenuWidget::ResolvePlayerController() const
{
        return Cast<AMO56PlayerController>(GetOwningPlayer());
}

