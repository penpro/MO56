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
#include "Net/UnrealNetwork.h"

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

        if (HasAuthority())
        {
                if (UMO56SaveSubsystem* SaveSubsystem = GetSaveSubsystem())
                {
                        SaveSubsystem->NotifyPlayerControllerReady(this);

                        if (APawn* ControlledPawn = GetPawn())
                        {
                                if (AMO56Character* ControlledCharacter = Cast<AMO56Character>(ControlledPawn))
                                {
                                        SaveSubsystem->RegisterPlayerCharacter(ControlledCharacter, this);
                                }
                        }
                }
        }
}

void AMO56PlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);

        DOREPLIFETIME(AMO56PlayerController, PlayerSaveId);
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

bool AMO56PlayerController::RequestLoadGame()
{
        if (HasAuthority())
        {
                return HandleLoadGameOnServer(TEXT(""), INDEX_NONE);
        }

        ServerLoadGame(TEXT(""), INDEX_NONE);
        return true;
}

bool AMO56PlayerController::RequestLoadGameBySlot(const FString& SlotName, int32 UserIndex)
{
        if (HasAuthority())
        {
                return HandleLoadGameOnServer(SlotName, UserIndex);
        }

        ServerLoadGame(SlotName, UserIndex);
        return true;
}

bool AMO56PlayerController::RequestCreateNewSaveSlot()
{
        if (HasAuthority())
        {
                return HandleCreateNewSaveSlot();
        }

        ServerCreateNewSaveSlot();
        return true;
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

void AMO56PlayerController::ServerLoadGame_Implementation(const FString& SlotName, int32 UserIndex)
{
        HandleLoadGameOnServer(SlotName, UserIndex);
}

void AMO56PlayerController::ServerCreateNewSaveSlot_Implementation()
{
        HandleCreateNewSaveSlot();
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

void AMO56PlayerController::ClientEnsureGameInput_Implementation()
{
        if (AMO56Character* MOCharacter = Cast<AMO56Character>(GetCharacter()))
        {
                MOCharacter->CloseAllPlayerMenus();
        }

        FInputModeGameOnly InputMode;
        SetInputMode(InputMode);

        SetShowMouseCursor(false);
        bEnableClickEvents = false;
        bEnableMouseOverEvents = false;
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

        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
                if (AMO56PlayerController* OtherController = Cast<AMO56PlayerController>(*It))
                {
                        OtherController->ClientEnsureGameInput();
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

bool AMO56PlayerController::HandleLoadGameOnServer(const FString& SlotName, int32 UserIndex)
{
        if (!HasAuthority())
        {
                return false;
        }

        bool bLoaded = false;

        if (UMO56SaveSubsystem* SaveSubsystem = GetSaveSubsystem())
        {
                if (!SlotName.IsEmpty())
                {
                        bLoaded = SaveSubsystem->LoadGameBySlot(SlotName, UserIndex);
                }
                else
                {
                        bLoaded = SaveSubsystem->LoadGame();
                }
        }

        if (bLoaded)
        {
                if (UWorld* World = GetWorld())
                {
                        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
                        {
                                if (AMO56PlayerController* OtherController = Cast<AMO56PlayerController>(*It))
                                {
                                        OtherController->ClientEnsureGameInput();
                                }
                        }
                }
        }

        return bLoaded;
}

bool AMO56PlayerController::HandleCreateNewSaveSlot()
{
        if (!HasAuthority())
        {
                return false;
        }

        if (UMO56SaveSubsystem* SaveSubsystem = GetSaveSubsystem())
        {
                const FSaveGameSummary Summary = SaveSubsystem->CreateNewSaveSlot();
                return !Summary.SlotName.IsEmpty();
        }

        return false;
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

void AMO56PlayerController::SetPlayerSaveId(const FGuid& InId)
{
        if (!HasAuthority())
        {
                return;
        }

        PlayerSaveId = InId;
        ForceNetUpdate();
}
