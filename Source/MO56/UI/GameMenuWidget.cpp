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
#include "MO56.h"

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

        if (CreateNewSaveButton)
        {
                CreateNewSaveButton->OnClicked.AddDynamic(this, &UGameMenuWidget::HandleCreateNewSaveClicked);
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
                        if (AMO56PlayerController* PC = ResolvePlayerController())
                        {
                                PC->RequestLoadGame();
                        }
                        else
                        {
                                UE_LOG(LogMO56, Error,
                                        TEXT("GameMenuWidget: No AMO56PlayerController available to request load. Configure"
                                             " PlayerControllerClass to BP_MO56PlayerController."));
                        }
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

void UGameMenuWidget::HandleCreateNewSaveClicked()
{
        EnsureSaveSubsystem();

        bool bCreated = false;

        if (AMO56PlayerController* PC = ResolvePlayerController())
        {
                bCreated = PC->RequestCreateNewSaveSlot();
        }
        else if (UMO56SaveSubsystem* SaveSubsystem = CachedSaveSubsystem.Get())
        {
                const FSaveGameSummary Summary = SaveSubsystem->CreateNewSaveSlot();
                bCreated = !Summary.SlotName.IsEmpty();
        }

        if (bCreated && FocusContainer && SaveGameMenuClass)
        {
                HandleLoadGameClicked();
        }
}

void UGameMenuWidget::HandleSaveGameClicked()
{
        ClearFocusWidget();
        if (AMO56PlayerController* PC = ResolvePlayerController())
        {
                PC->RequestSaveGame();
        }
        else
        {
                EnsureSaveSubsystem();

                if (UMO56SaveSubsystem* SaveSubsystem = CachedSaveSubsystem.Get())
                {
                        if (UWorld* World = GetWorld())
                        {
                                if (World->GetNetMode() != NM_Client)
                                {
                                SaveSubsystem->SaveCurrentGame();
                                }
                                else
                                {
                                        UE_LOG(LogMO56, Warning, TEXT("HandleSaveGameClicked: Client without controller cannot directly invoke SaveGame."));
                                }
                        }
                }
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
                        SaveSubsystem->SaveCurrentGame();
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

