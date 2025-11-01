// Implementation: Extend this controller in blueprints to configure input mappings and UI.
// Menu widgets call the BlueprintCallable helpers, which forward to the server and manage
// save/load as well as pawn possession for multiplayer.
#include "MO56PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "MO56.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "InventoryComponent.h"
#include "InventoryContainer.h"
#include "MO56Character.h"
#include "Save/MO56SaveSubsystem.h"

void AMO56PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogMO56, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AMO56PlayerController::SetupInputComponent()
{
        Super::SetupInputComponent();

        if (IsLocalPlayerController())
        {
                if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
                {
                        for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
                        {
                                Subsystem->AddMappingContext(CurrentContext, 0);
                        }

                        if (!SVirtualJoystick::ShouldDisplayTouchInterface())
                        {
                                for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
                                {
                                        Subsystem->AddMappingContext(CurrentContext, 0);
                                }
                        }
                }
        }
}

void AMO56PlayerController::RequestNewGame()
{
        if (!HasAuthority())
        {
                UE_LOG(LogMO56, Warning, TEXT("RequestNewGame ignored on non-authority controller."));
                return;
        }

        const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
        HandleNewGameOnServer(LevelName);
}

void AMO56PlayerController::RequestSaveGame()
{
        if (HasAuthority())
        {
                HandleSaveGameOnServer(false);
        }
        else
        {
                ServerSaveGame();
        }
}

void AMO56PlayerController::RequestSaveAndExit()
{
        if (HasAuthority())
        {
                HandleSaveGameOnServer(true);
        }
        else
        {
                ServerSaveAndExit();
        }
}

void AMO56PlayerController::RequestPossessPawn(APawn* TargetPawn)
{
        if (!TargetPawn)
        {
                return;
        }

        if (HasAuthority())
        {
                HandlePossessPawn(TargetPawn);
        }
        else
        {
                ServerPossessPawn(TargetPawn);
        }
}

void AMO56PlayerController::RequestOpenPawnInventory(APawn* TargetPawn)
{
        if (!TargetPawn)
        {
                return;
        }

        if (HasAuthority())
        {
                HandleOpenPawnInventory(TargetPawn);
        }
        else
        {
                ServerOpenPawnInventory(TargetPawn);
        }
}

void AMO56PlayerController::NotifyPawnContextFocus(APawn* TargetPawn, bool bHasFocus)
{
        if (HasAuthority())
        {
                HandlePawnContext(TargetPawn, bHasFocus);
        }
        else
        {
                ServerSetPawnContext(TargetPawn, bHasFocus);
        }
}

void AMO56PlayerController::ServerExecuteNewGame_Implementation(const FString& LevelName)
{
        HandleNewGameOnServer(LevelName);
}

void AMO56PlayerController::ServerSaveGame_Implementation()
{
        HandleSaveGameOnServer(false);
}

void AMO56PlayerController::ServerSaveAndExit_Implementation()
{
        HandleSaveGameOnServer(true);
}

void AMO56PlayerController::ServerPossessPawn_Implementation(APawn* TargetPawn)
{
        HandlePossessPawn(TargetPawn);
}

void AMO56PlayerController::ServerOpenPawnInventory_Implementation(APawn* TargetPawn)
{
        HandleOpenPawnInventory(TargetPawn);
        ClientOpenPawnInventoryResponse(TargetPawn);
}

void AMO56PlayerController::ServerSetPawnContext_Implementation(APawn* TargetPawn, bool bHasFocus)
{
        HandlePawnContext(TargetPawn, bHasFocus);
}

void AMO56PlayerController::ClientOpenPawnInventoryResponse_Implementation(APawn* TargetPawn)
{
        HandleOpenPawnInventory(TargetPawn);
}

void AMO56PlayerController::ClientOpenContainerInventory_Implementation(AInventoryContainer* ContainerActor)
{
        AMO56Character* OwningCharacter = Cast<AMO56Character>(GetCharacter());
        if (!OwningCharacter)
        {
                return;
        }

        UInventoryComponent* ContainerInventory = nullptr;
        if (ContainerActor)
        {
                ContainerInventory = ContainerActor->GetInventoryComponent();

                if (!ContainerInventory)
                {
                        ContainerInventory = ContainerActor->FindComponentByClass<UInventoryComponent>();
                }
        }

        OwningCharacter->OpenContainerInventory(ContainerInventory, ContainerActor);
}

void AMO56PlayerController::HandleNewGameOnServer(const FString& LevelName)
{
        if (!HasAuthority())
        {
                return;
        }

        if (UMO56SaveSubsystem* SaveSubsystem = GetSaveSubsystem())
        {
                SaveSubsystem->ResetToNewGame();
        }

        UWorld* World = GetWorld();
        if (!World)
        {
                return;
        }

        FString DesiredLevel = LevelName;
        if (DesiredLevel.IsEmpty())
        {
                DesiredLevel = UGameplayStatics::GetCurrentLevelName(World, true);
        }

        if (DesiredLevel.IsEmpty())
        {
                UE_LOG(LogMO56, Warning, TEXT("HandleNewGameOnServer: No valid level name."));
                return;
        }

        FString TravelURL = DesiredLevel;
        if (World->GetNetMode() != NM_DedicatedServer)
        {
                const bool bHasOptions = TravelURL.Contains(TEXT("?"));
                const bool bHasListen = TravelURL.Contains(TEXT("listen"));
                if (!bHasListen)
                {
                        TravelURL += bHasOptions ? TEXT("&listen") : TEXT("?listen");
                }
        }

        UE_LOG(LogMO56, Display, TEXT("Starting new game on level %s"), *TravelURL);
        World->ServerTravel(TravelURL, true);
}

void AMO56PlayerController::HandleSaveGameOnServer(bool bAlsoExit)
{
        if (!HasAuthority())
        {
                return;
        }

        if (UMO56SaveSubsystem* SaveSubsystem = GetSaveSubsystem())
        {
                SaveSubsystem->SaveGame();
        }

        if (bAlsoExit)
        {
                ConsoleCommand(TEXT("quit"));
        }
}

void AMO56PlayerController::HandlePossessPawn(APawn* TargetPawn)
{
        if (!HasAuthority() || !TargetPawn)
        {
                return;
        }

        if (TargetPawn == GetPawn())
        {
                return;
        }

        if (AController* ExistingController = TargetPawn->GetController())
        {
                if (ExistingController != this)
                {
                        ExistingController->UnPossess();
                }
        }

        Possess(TargetPawn);
}

void AMO56PlayerController::HandleOpenPawnInventory(APawn* TargetPawn)
{
        if (!IsLocalController() || !TargetPawn)
        {
                return;
        }

        AMO56Character* OwningCharacter = Cast<AMO56Character>(GetCharacter());
        if (!OwningCharacter)
        {
                return;
        }

        if (UInventoryComponent* Inventory = TargetPawn->FindComponentByClass<UInventoryComponent>())
        {
                OwningCharacter->OpenContainerInventory(Inventory, TargetPawn);
        }
}

void AMO56PlayerController::HandlePawnContext(APawn* TargetPawn, bool bHasFocus)
{
        if (!HasAuthority() || !TargetPawn)
        {
                return;
        }

    if (AMO56Character* TargetCharacter = Cast<AMO56Character>(TargetPawn))
    {
        TargetCharacter->SetEnableAI(!bHasFocus);
    }
}

UMO56SaveSubsystem* AMO56PlayerController::GetSaveSubsystem() const
{
        if (UGameInstance* GameInstance = GetGameInstance())
        {
                return GameInstance->GetSubsystem<UMO56SaveSubsystem>();
        }

        return nullptr;
}
