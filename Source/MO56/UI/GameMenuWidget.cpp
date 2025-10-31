#include "UI/GameMenuWidget.h"

#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Save/MO56SaveSubsystem.h"

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

        if (ExitGameButton)
        {
                ExitGameButton->OnClicked.AddDynamic(this, &UGameMenuWidget::HandleExitGameClicked);
        }
}

void UGameMenuWidget::NativeConstruct()
{
        Super::NativeConstruct();
        EnsureSaveSubsystem();
}

void UGameMenuWidget::SetSaveSubsystem(UMO56SaveSubsystem* Subsystem)
{
        CachedSaveSubsystem = Subsystem;
}

void UGameMenuWidget::HandleNewGameClicked()
{
        EnsureSaveSubsystem();
        if (UMO56SaveSubsystem* SaveSubsystem = CachedSaveSubsystem.Get())
        {
                SaveSubsystem->ResetToNewGame();
        }
}

void UGameMenuWidget::HandleLoadGameClicked()
{
        EnsureSaveSubsystem();
        if (UMO56SaveSubsystem* SaveSubsystem = CachedSaveSubsystem.Get())
        {
                SaveSubsystem->LoadGame();
        }
}

void UGameMenuWidget::HandleExitGameClicked()
{
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

