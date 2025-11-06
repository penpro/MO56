// Implementation: Extend this controller in blueprints to configure input mappings and UI.
// Menu widgets call the BlueprintCallable helpers, which forward to the server and manage
// save/load as well as pawn possession for multiplayer.
#include "MO56PlayerController.h"
#include "EnhancedInputSubsystemInterface.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "MO56.h"
#include "MO56DebugLogSubsystem.h"
#include "Util/MO56NetDebug.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Misc/EngineVersionComparison.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/SpectatorPawn.h"
#include "Engine/World.h"
#include "InventoryComponent.h"
#include "InventoryContainer.h"
#include "MO56Character.h"
#include "MO56PossessionMenuManagerSubsystem.h"
#include "Save/MO56SaveSubsystem.h"
#include "EngineUtils.h"
#include "Components/MOPersistentPawnComponent.h"
#include "Net/UnrealNetwork.h"
#include "UI/MO56PossessMenuWidget.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "GameFramework/PawnMovementComponent.h"
#include "TimerManager.h"

namespace
{
        FString DescribePawnForDebug(const APawn* Pawn)
        {
                if (!Pawn)
                {
                        return TEXT("Pawn=None");
                }

                const FGuid PawnId = MO56ResolvePawnId(Pawn);
                const FString IdString = PawnId.IsValid() ? PawnId.ToString(EGuidFormats::DigitsWithHyphens) : TEXT("None");
                return FString::Printf(TEXT("Pawn=%s Id=%s"), *GetNameSafe(Pawn), *IdString);
        }

        FString DescribeInputState(const AMO56PlayerController& Controller)
        {
                const bool bLeftMouseDown = Controller.IsInputKeyDown(EKeys::LeftMouseButton);

                return FString::Printf(TEXT("Input(Mouse=%s Click=%s Over=%s IgnoreLook=%s IgnoreMove=%s LeftMouseDown=%s Mode=%s)"),
                        Controller.bShowMouseCursor ? TEXT("true") : TEXT("false"),
                        Controller.bEnableClickEvents ? TEXT("true") : TEXT("false"),
                        Controller.bEnableMouseOverEvents ? TEXT("true") : TEXT("false"),
                        Controller.IsLookInputIgnored() ? TEXT("true") : TEXT("false"),
                        Controller.IsMoveInputIgnored() ? TEXT("true") : TEXT("false"),
                        bLeftMouseDown ? TEXT("true") : TEXT("false"),
                        *Controller.DescribeDebugInputMode());
        }

        void LogActiveContexts(AMO56PlayerController* Controller)
        {
                if (!Controller)
                {
                        return;
                }

                const TArray<FTrackedInputContext>& Active = Controller->GetTrackedActiveContexts();

                FString Names;
                for (const FTrackedInputContext& Entry : Active)
                {
                        const UInputMappingContext* Context = Entry.Context.Get();
                        if (!Context)
                        {
                                continue;
                        }

                        if (!Names.IsEmpty())
                        {
                                Names += TEXT(" ");
                        }

                        Names += FString::Printf(TEXT("%s@%d"), *Context->GetName(), Entry.Priority);
                }

                if (Names.IsEmpty())
                {
                        Names = TEXT("<none>");
                }

                UE_LOG(LogTemp, Log, TEXT("[Input] Contexts(tracked): %s"), *Names);
        }
}

void AMO56PlayerController::BeginPlay()
{
        Super::BeginPlay();

        LogDebugEvent(TEXT("BeginPlay"), FString::Printf(TEXT("Class=%s Local=%s Authority=%s %s %s"),
                *GetClass()->GetName(),
                IsLocalPlayerController() ? TEXT("true") : TEXT("false"),
                HasAuthority() ? TEXT("true") : TEXT("false"),
                *DescribePawnForDebug(GetPawn()),
                *DescribeInputState(*this)));

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

        if (IsLocalPlayerController())
        {
                if (UGameInstance* GameInstance = GetGameInstance())
                {
                        if (UMO56PossessionMenuManagerSubsystem* MenuSubsystem = GameInstance->GetSubsystem<UMO56PossessionMenuManagerSubsystem>())
                        {
                                MenuSubsystem->EnsureBindingsForAllLocalPlayers();
                                if (GetPawn() == nullptr || PlayerState == nullptr)
                                {
                                        if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
                                        {
                                                MenuSubsystem->EnsureMenuOpenForLocalPlayer(LocalPlayer);
                                        }
                                }
                        }
                }

                if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
                {
                        static const TCHAR* PossessionActionPath = TEXT("/Game/Input/Actions/IA_Possession.IA_Possession");
                        static const TCHAR* PossessionActionFallbackPath = TEXT("/Game/Input/IA_Possession.IA_Possession");

                        UInputAction* PossessionAction = LoadObject<UInputAction>(nullptr, PossessionActionPath);
                        if (!PossessionAction)
                        {
                                PossessionAction = LoadObject<UInputAction>(nullptr, PossessionActionFallbackPath);
                        }

                        if (PossessionAction)
                        {
                                EnhancedInput->BindAction(PossessionAction, ETriggerEvent::Started, this, &AMO56PlayerController::OnIA_Possession);
                        }
                }

                if (GetPawn() == nullptr || PlayerState == nullptr)
                {
                        ServerQueryPossessablePawns();
                }
        }
}

void AMO56PlayerController::OnPossess(APawn* InPawn)
{
        Super::OnPossess(InPawn);

        const ENetRole LocalRole = InPawn ? InPawn->GetLocalRole() : ROLE_None;
        const ENetRole ObservedRemoteRole = InPawn ? InPawn->GetRemoteRole() : ROLE_None;
        const FString RoleDetail = FString::Printf(TEXT("Roles(Local=%s Remote=%s ControllerIsLocal=%s)"),
                MO56RoleToString(LocalRole),
                MO56RoleToString(ObservedRemoteRole),
                IsLocalPlayerController() ? TEXT("true") : TEXT("false"));

        LogDebugEvent(TEXT("OnPossess"), FString::Printf(TEXT("New=%s %s %s"),
                *DescribePawnForDebug(InPawn),
                *DescribeInputState(*this),
                *RoleDetail), InPawn);

        if (IsLocalPlayerController())
        {
                if (UGameInstance* GameInstance = GetGameInstance())
                {
                        if (UMO56PossessionMenuManagerSubsystem* MenuSubsystem = GameInstance->GetSubsystem<UMO56PossessionMenuManagerSubsystem>())
                        {
                                if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
                                {
                                        MenuSubsystem->EnsureMenuClosedForLocalPlayer(LocalPlayer);
                                }
                        }
                }

                ReapplyEnhancedInputContexts();
                ApplyGameplayInputState();
                LogActiveContexts(this);
        }

        SetIgnoreLookInput(false);
        SetIgnoreMoveInput(false);

        if (HasAuthority())
        {
                ClientReapplyEnhancedInputContexts();
                ClientEnsureGameInput();
                ClientValidatePostPossess(InPawn);
                ClientPostRestartValidate();
        }
}

void AMO56PlayerController::OnUnPossess()
{
        APawn* PreviousPawn = GetPawn();
        Super::OnUnPossess();

        LogDebugEvent(TEXT("OnUnPossess"), FString::Printf(TEXT("Previous=%s %s"),
                *DescribePawnForDebug(PreviousPawn),
                *DescribeInputState(*this)), PreviousPawn);

        LastContainerOwningCharacter.Reset();

        if (IsLocalPlayerController())
        {
                if (UGameInstance* GameInstance = GetGameInstance())
                {
                        if (UMO56PossessionMenuManagerSubsystem* MenuSubsystem = GameInstance->GetSubsystem<UMO56PossessionMenuManagerSubsystem>())
                        {
                                if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
                                {
                                        MenuSubsystem->EnsureMenuOpenForLocalPlayer(LocalPlayer);
                                }
                        }
                }
                ServerQueryPossessablePawns();
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

        LogDebugEvent(TEXT("SetupInputComponent"), FString::Printf(TEXT("Local=%s %s"),
                IsLocalPlayerController() ? TEXT("true") : TEXT("false"),
                *DescribeInputState(*this)));

        EnsureDefaultInputContexts();
}

void AMO56PlayerController::RequestNewGame()
{
        LogDebugEvent(TEXT("RequestNewGame"), FString::Printf(TEXT("Authority=%s"), HasAuthority() ? TEXT("true") : TEXT("false")));

        if (!HasAuthority())
        {
                ServerExecuteNewGame(TEXT("TestLevel"));
                return;
        }

        if (UMO56SaveSubsystem* SaveSubsystem = GetSaveSubsystem())
        {
                SaveSubsystem->StartNewGame(TEXT("TestLevel"));
        }
}

void AMO56PlayerController::RequestSaveGame()
{
        LogDebugEvent(TEXT("RequestSaveGame"), FString::Printf(TEXT("Authority=%s"), HasAuthority() ? TEXT("true") : TEXT("false")));

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
        LogDebugEvent(TEXT("RequestSaveAndExit"), FString::Printf(TEXT("Authority=%s"), HasAuthority() ? TEXT("true") : TEXT("false")));

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
        LogDebugEvent(TEXT("RequestLoadGame"), FString::Printf(TEXT("Authority=%s"), HasAuthority() ? TEXT("true") : TEXT("false")));

        if (HasAuthority())
        {
                return HandleLoadGameOnServer(TEXT(""), INDEX_NONE);
        }

        ServerLoadGame(TEXT(""), INDEX_NONE);
        return true;
}

bool AMO56PlayerController::RequestLoadGameBySlot(const FString& SlotName, int32 UserIndex)
{
        LogDebugEvent(TEXT("RequestLoadGameBySlot"), FString::Printf(TEXT("Authority=%s Slot=%s User=%d"), HasAuthority() ? TEXT("true") : TEXT("false"), *SlotName, UserIndex));

        if (HasAuthority())
        {
                return HandleLoadGameOnServer(SlotName, UserIndex);
        }

        ServerLoadGame(SlotName, UserIndex);
        return true;
}

bool AMO56PlayerController::RequestLoadGameById(const FGuid& SaveId)
{
        LogDebugEvent(TEXT("RequestLoadGameById"), FString::Printf(TEXT("Authority=%s SaveId=%s"), HasAuthority() ? TEXT("true") : TEXT("false"), *SaveId.ToString(EGuidFormats::DigitsWithHyphens)));

        if (!SaveId.IsValid())
        {
                return false;
        }

        if (HasAuthority())
        {
                return HandleLoadGameByIdOnServer(SaveId);
        }

        ServerLoadGameById(SaveId);
        return true;
}

bool AMO56PlayerController::RequestCreateNewSaveSlot()
{
        LogDebugEvent(TEXT("RequestCreateNewSaveSlot"), FString::Printf(TEXT("Authority=%s"), HasAuthority() ? TEXT("true") : TEXT("false")));

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

        LogDebugEvent(TEXT("RequestPossessPawn"), FString::Printf(TEXT("Target=%s Authority=%s"),
                *DescribePawnForDebug(TargetPawn),
                HasAuthority() ? TEXT("true") : TEXT("false")), TargetPawn);

        if (HasAuthority())
        {
                HandlePossessPawn(TargetPawn);
        }
        else
        {
                ServerPossessPawn(TargetPawn);
        }
}

void AMO56PlayerController::ServerQueryPossessablePawns_Implementation()
{
        LogDebugEvent(TEXT("ServerQueryPossessablePawns"), FString::Printf(TEXT("Authority=%s"), HasAuthority() ? TEXT("true") : TEXT("false")));

        if (UGameInstance* GameInstance = GetGameInstance())
        {
                if (UMO56SaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>())
                {
                        TArray<FMOPossessablePawnInfo> List;
                        SaveSubsystem->BuildPossessablePawnList(this, List);
                        ClientReceivePossessablePawns(List);
                }
        }
}

void AMO56PlayerController::ClientReceivePossessablePawns_Implementation(const TArray<FMOPossessablePawnInfo>& List)
{
        CachedPossessablePawns = List;

        LogDebugEvent(TEXT("ClientReceivePossessablePawns"), FString::Printf(TEXT("Count=%d"), List.Num()));

        if (IsLocalPlayerController())
        {
                if (UGameInstance* GameInstance = GetGameInstance())
                {
                        if (UMO56PossessionMenuManagerSubsystem* MenuSubsystem = GameInstance->GetSubsystem<UMO56PossessionMenuManagerSubsystem>())
                        {
                                if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
                                {
                                        MenuSubsystem->RefreshMenuForLocalPlayer(LocalPlayer, CachedPossessablePawns);
                                }
                        }
                }
        }
}

void AMO56PlayerController::ServerRequestPossessPawnById_Implementation(const FGuid& PawnId)
{
        LogDebugEvent(TEXT("ServerRequestPossessPawnById"),
                FString::Printf(TEXT("PawnId=%s"), *PawnId.ToString(EGuidFormats::DigitsWithHyphens)));

        if (!PawnId.IsValid())
        {
                return;
        }

        if (UGameInstance* GameInstance = GetGameInstance())
        {
                if (UMO56SaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>())
                {
                        FString Reason;
                        if (SaveSubsystem->TryAssignAndPossess(this, PawnId, Reason))
                        {
                                return;
                        }

                        APawn* TargetPawn = nullptr;
                        if (UWorld* World = GetWorld())
                        {
                                for (TActorIterator<APawn> It(World); It; ++It)
                                {
                                        if (UMOPersistentPawnComponent* Persist = It->FindComponentByClass<UMOPersistentPawnComponent>())
                                        {
                                                if (Persist->GetPawnId() == PawnId)
                                                {
                                                        TargetPawn = *It;
                                                        break;
                                                }
                                        }
                                }
                        }

                        if (TargetPawn)
                        {
                                LogDebugEvent(TEXT("ServerRequestPossessPawnByIdFallback"),
                                        FString::Printf(TEXT("FallbackToDirectPossess Target=%s"), *GetNameSafe(TargetPawn)), TargetPawn);

                                HandlePossessPawn(TargetPawn);
                                return;
                        }

                        ClientMessage(FString::Printf(TEXT("Cannot possess: %s"), *Reason));
                }
        }
}

void AMO56PlayerController::OpenPossessMenu()
{
        if (!IsLocalPlayerController())
        {
                return;
        }

        LogDebugEvent(TEXT("OpenPossessMenu"), FString::Printf(TEXT("MenuState=%s"), *DescribeInputState(*this)));

        if (UGameInstance* GameInstance = GetGameInstance())
        {
                if (UMO56PossessionMenuManagerSubsystem* MenuSubsystem = GameInstance->GetSubsystem<UMO56PossessionMenuManagerSubsystem>())
                {
                        if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
                        {
                                MenuSubsystem->EnsureMenuOpenForLocalPlayer(LocalPlayer);
                        }
                }
        }

        ApplyMenuInputState();
}

void AMO56PlayerController::ClosePossessMenu()
{
        if (!IsLocalPlayerController())
        {
                return;
        }

        LogDebugEvent(TEXT("ClosePossessMenu"), FString::Printf(TEXT("MenuState=%s"), *DescribeInputState(*this)));

        if (UGameInstance* GameInstance = GetGameInstance())
        {
                if (UMO56PossessionMenuManagerSubsystem* MenuSubsystem = GameInstance->GetSubsystem<UMO56PossessionMenuManagerSubsystem>())
                {
                        if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
                        {
                                MenuSubsystem->EnsureMenuClosedForLocalPlayer(LocalPlayer);
                        }
                }
        }

        EnsureGameplayInputMode();
}

void AMO56PlayerController::OnIA_Possession()
{
        LogDebugEvent(TEXT("InputPossession"), FString::Printf(TEXT("MenuToggle Requested")));

        if (UGameInstance* GameInstance = GetGameInstance())
        {
                if (UMO56PossessionMenuManagerSubsystem* MenuSubsystem = GameInstance->GetSubsystem<UMO56PossessionMenuManagerSubsystem>())
                {
                        if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
                        {
                                const bool bMenuOpen = MenuSubsystem->IsMenuOpenForLocalPlayer(LocalPlayer);
                                if (bMenuOpen)
                                {
                                        ClosePossessMenu();
                                }
                                else
                                {
                                        OpenPossessMenu();
                                        ServerQueryPossessablePawns();
                                }
                        }
                }
        }
}

void AMO56PlayerController::RequestOpenPawnInventory(APawn* TargetPawn)
{
        if (!TargetPawn)
        {
                return;
        }

        LogDebugEvent(TEXT("RequestOpenPawnInventory"), FString::Printf(TEXT("Target=%s"), *DescribePawnForDebug(TargetPawn)), TargetPawn);

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
        LogDebugEvent(TEXT("NotifyPawnContextFocus"), FString::Printf(TEXT("Target=%s Focus=%s"), *DescribePawnForDebug(TargetPawn), bHasFocus ? TEXT("true") : TEXT("false")), TargetPawn);

        if (HasAuthority())
        {
                HandlePawnContext(TargetPawn, bHasFocus);
        }
        else
        {
                ServerSetPawnContext(TargetPawn, bHasFocus);
        }
}

void AMO56PlayerController::RequestContainerInventoryOwnership(AInventoryContainer* ContainerActor)
{
        if (!ContainerActor)
        {
                return;
        }

        LogDebugEvent(TEXT("RequestContainerOwnership"), FString::Printf(TEXT("Container=%s"), *GetNameSafe(ContainerActor)));

        if (HasAuthority())
        {
                ContainerActor->SetOwner(this);
        }
        else
        {
                ServerRequestContainerInventoryOwnership(ContainerActor);
        }
}

void AMO56PlayerController::NotifyContainerInventoryClosed(AInventoryContainer* ContainerActor)
{
        if (!ContainerActor)
        {
                return;
        }

        AMO56Character* OwningCharacter = LastContainerOwningCharacter.Get();
        LogDebugEvent(TEXT("NotifyContainerInventoryClosed"), FString::Printf(TEXT("Container=%s OwningPawn=%s"), *GetNameSafe(ContainerActor), *DescribePawnForDebug(OwningCharacter)), OwningCharacter);

        if (HasAuthority())
        {
                if (!OwningCharacter)
                {
                        OwningCharacter = Cast<AMO56Character>(GetCharacter());
                }

                if (OwningCharacter)
                {
                        ContainerActor->NotifyInventoryClosed(OwningCharacter);
                        OwningCharacter->CloseContainerInventoryForActor(ContainerActor, false);
                }
        }
        else
        {
                ServerNotifyContainerInventoryClosed(ContainerActor);
        }

        if (OwningCharacter && LastContainerOwningCharacter.Get() == OwningCharacter)
        {
                LastContainerOwningCharacter.Reset();
        }
}

void AMO56PlayerController::ServerExecuteNewGame_Implementation(const FString& LevelName)
{
        LogDebugEvent(TEXT("ServerExecuteNewGame"), FString::Printf(TEXT("Level=%s"), *LevelName));
        HandleNewGameOnServer(LevelName);
}

void AMO56PlayerController::ServerSaveGame_Implementation()
{
        LogDebugEvent(TEXT("ServerSaveGame"), FString::Printf(TEXT("Authority=%s"), HasAuthority() ? TEXT("true") : TEXT("false")));
        HandleSaveGameOnServer(false);
}

void AMO56PlayerController::ServerSaveAndExit_Implementation()
{
        LogDebugEvent(TEXT("ServerSaveAndExit"), FString::Printf(TEXT("Authority=%s"), HasAuthority() ? TEXT("true") : TEXT("false")));
        HandleSaveGameOnServer(true);
}

void AMO56PlayerController::ServerLoadGame_Implementation(const FString& SlotName, int32 UserIndex)
{
        LogDebugEvent(TEXT("ServerLoadGame"), FString::Printf(TEXT("Slot=%s User=%d"), *SlotName, UserIndex));
        HandleLoadGameOnServer(SlotName, UserIndex);
}

void AMO56PlayerController::ServerLoadGameById_Implementation(const FGuid& SaveId)
{
        LogDebugEvent(TEXT("ServerLoadGameById"), FString::Printf(TEXT("SaveId=%s"), *SaveId.ToString(EGuidFormats::DigitsWithHyphens)));
        HandleLoadGameByIdOnServer(SaveId);
}

void AMO56PlayerController::ServerCreateNewSaveSlot_Implementation()
{
        LogDebugEvent(TEXT("ServerCreateNewSaveSlot"), FString(TEXT("Request")));
        HandleCreateNewSaveSlot();
}

void AMO56PlayerController::ServerPossessPawn_Implementation(APawn* TargetPawn)
{
        LogDebugEvent(TEXT("ServerPossessPawn"), FString::Printf(TEXT("Target=%s"), *DescribePawnForDebug(TargetPawn)), TargetPawn);
        HandlePossessPawn(TargetPawn);
}

void AMO56PlayerController::ServerOpenPawnInventory_Implementation(APawn* TargetPawn)
{
        LogDebugEvent(TEXT("ServerOpenPawnInventory"), FString::Printf(TEXT("Target=%s"), *DescribePawnForDebug(TargetPawn)), TargetPawn);
        HandleOpenPawnInventory(TargetPawn);
        ClientOpenPawnInventoryResponse(TargetPawn);
}

void AMO56PlayerController::ServerSetPawnContext_Implementation(APawn* TargetPawn, bool bHasFocus)
{
        LogDebugEvent(TEXT("ServerSetPawnContext"), FString::Printf(TEXT("Target=%s Focus=%s"), *DescribePawnForDebug(TargetPawn), bHasFocus ? TEXT("true") : TEXT("false")), TargetPawn);
        HandlePawnContext(TargetPawn, bHasFocus);
}

void AMO56PlayerController::ServerRequestContainerInventoryOwnership_Implementation(AInventoryContainer* ContainerActor)
{
        if (!ContainerActor)
        {
                return;
        }

        LogDebugEvent(TEXT("ServerRequestContainerOwnership"), FString::Printf(TEXT("Container=%s"), *GetNameSafe(ContainerActor)));

        ContainerActor->SetOwner(this);
}

void AMO56PlayerController::ServerNotifyContainerInventoryClosed_Implementation(AInventoryContainer* ContainerActor)
{
        if (!ContainerActor)
        {
                return;
        }

        AMO56Character* OwningCharacter = LastContainerOwningCharacter.Get();
        if (!OwningCharacter)
        {
                OwningCharacter = Cast<AMO56Character>(GetCharacter());
        }

        LogDebugEvent(TEXT("ServerNotifyContainerInventoryClosed"), FString::Printf(TEXT("Container=%s OwningPawn=%s"), *GetNameSafe(ContainerActor), *DescribePawnForDebug(OwningCharacter)), OwningCharacter);

        if (OwningCharacter)
        {
                ContainerActor->NotifyInventoryClosed(OwningCharacter);
                OwningCharacter->CloseContainerInventoryForActor(ContainerActor, false);
        }

        if (OwningCharacter && LastContainerOwningCharacter.Get() == OwningCharacter)
        {
                LastContainerOwningCharacter.Reset();
        }
}

void AMO56PlayerController::ClientOpenPawnInventoryResponse_Implementation(APawn* TargetPawn)
{
        LogDebugEvent(TEXT("ClientOpenPawnInventoryResponse"), FString::Printf(TEXT("Target=%s"), *DescribePawnForDebug(TargetPawn)), TargetPawn);
        HandleOpenPawnInventory(TargetPawn);
}

void AMO56PlayerController::ClientOpenContainerInventory_Implementation(AInventoryContainer* ContainerActor)
{
        LogDebugEvent(TEXT("ClientOpenContainerInventory"), FString::Printf(TEXT("Container=%s"), *GetNameSafe(ContainerActor)));

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
        SetLastContainerOwningCharacter(OwningCharacter);
}

void AMO56PlayerController::ClientCloseContainerInventory_Implementation(AInventoryContainer* ContainerActor)
{
        LogDebugEvent(TEXT("ClientCloseContainerInventory"), FString::Printf(TEXT("Container=%s"), *GetNameSafe(ContainerActor)));

        if (AMO56Character* OwningCharacter = Cast<AMO56Character>(GetCharacter()))
        {
                OwningCharacter->CloseContainerInventoryForActor(ContainerActor);
        }
}

void AMO56PlayerController::ClientRestart_Implementation(APawn* NewPawn)
{
        Super::ClientRestart_Implementation(NewPawn);

        const ENetRole LocalRole = NewPawn ? NewPawn->GetLocalRole() : ROLE_None;
        const ENetRole ObservedRemoteRole = NewPawn ? NewPawn->GetRemoteRole() : ROLE_None;
        const bool bPawnLocallyControlled = NewPawn ? NewPawn->IsLocallyControlled() : false;
        const ENetMode ControllerNetMode = GetNetMode();
        const ENetMode WorldNetMode = GetWorld() ? GetWorld()->GetNetMode() : NM_Standalone;
        const ENetMode PawnNetMode = NewPawn ? NewPawn->GetNetMode() : ControllerNetMode;
        const TCHAR* MovementNetModeString = TEXT("NM_Unknown");
        if (UPawnMovementComponent* MovementComponent = NewPawn ? NewPawn->GetMovementComponent() : nullptr)
        {
                MovementNetModeString = MO56NetModeToString(MovementComponent->GetNetMode());
        }

        LogDebugEvent(TEXT("ClientRestart"), FString::Printf(TEXT("Pawn=%s Roles(Local=%s Remote=%s) PawnLocal=%s ControllerLocal=%s ControllerNetMode=%s WorldNetMode=%s PawnNetMode=%s MovementNetMode=%s"),
                *DescribePawnForDebug(NewPawn),
                MO56RoleToString(LocalRole),
                MO56RoleToString(ObservedRemoteRole),
                bPawnLocallyControlled ? TEXT("true") : TEXT("false"),
                IsLocalPlayerController() ? TEXT("true") : TEXT("false"),
                MO56NetModeToString(ControllerNetMode),
                MO56NetModeToString(WorldNetMode),
                MO56NetModeToString(PawnNetMode),
                MovementNetModeString), NewPawn);

        ReapplyEnhancedInputContexts();
        ApplyGameplayInputState();

        UE_LOG(LogTemp, Log, TEXT("[Input] ClientRestart PawnRole=%s PawnLocal=%s ControllerLocal=%s ControllerNetMode=%s WorldNetMode=%s Cursor=%s MoveIgnored=%s LookIgnored=%s"),
                MO56RoleToString(LocalRole),
                bPawnLocallyControlled ? TEXT("true") : TEXT("false"),
                IsLocalPlayerController() ? TEXT("true") : TEXT("false"),
                MO56NetModeToString(ControllerNetMode),
                MO56NetModeToString(WorldNetMode),
                bShowMouseCursor ? TEXT("true") : TEXT("false"),
                IsMoveInputIgnored() ? TEXT("true") : TEXT("false"),
                IsLookInputIgnored() ? TEXT("true") : TEXT("false"));
        LogActiveContexts(this);
}

void AMO56PlayerController::ClientEnsureGameInput_Implementation()
{
        LogDebugEvent(TEXT("ClientEnsureGameInput"), FString::Printf(TEXT("Before %s"), *DescribeInputState(*this)));

        EnsureGameInputLocal();

        LogDebugEvent(TEXT("ClientEnsureGameInputComplete"), FString::Printf(TEXT("After %s"), *DescribeInputState(*this)));
}

void AMO56PlayerController::ClientReapplyEnhancedInputContexts_Implementation()
{
        LogDebugEvent(TEXT("ClientReapplyInputContexts"), FString::Printf(TEXT("Before %s"), *DescribeInputState(*this)));

        ReapplyEnhancedInputContexts();

        LogDebugEvent(TEXT("ClientReapplyInputContextsComplete"), FString::Printf(TEXT("After %s"), *DescribeInputState(*this)));
}

void AMO56PlayerController::ClientValidatePostPossess_Implementation(APawn* TargetPawn)
{
        const FString PawnDescription = DescribePawnForDebug(TargetPawn);
        const ENetRole LocalRole = TargetPawn ? TargetPawn->GetLocalRole() : ROLE_None;
        const ENetRole ObservedRemoteRole = TargetPawn ? TargetPawn->GetRemoteRole() : ROLE_None;
        const bool bLocallyControlled = TargetPawn ? TargetPawn->IsLocallyControlled() : false;
        const ENetMode ControllerNetMode = GetNetMode();
        const ENetMode WorldNetMode = GetWorld() ? GetWorld()->GetNetMode() : NM_Standalone;
        const ENetMode PawnNetMode = TargetPawn ? TargetPawn->GetNetMode() : ControllerNetMode;
        const TCHAR* MovementNetModeString = TEXT("NM_Unknown");
        if (UPawnMovementComponent* MovementComponent = TargetPawn ? TargetPawn->GetMovementComponent() : nullptr)
        {
                MovementNetModeString = MO56NetModeToString(MovementComponent->GetNetMode());
        }

        const bool bClientContext = (ControllerNetMode == NM_Client);
        const bool bGood = (bClientContext && LocalRole == ROLE_AutonomousProxy)
                || (!bClientContext && LocalRole == ROLE_Authority)
                || bLocallyControlled;

        LogDebugEvent(TEXT("ClientValidatePostPossess"), FString::Printf(TEXT("Pawn=%s Roles(Local=%s Remote=%s) PawnLocal=%s ControllerNetMode=%s WorldNetMode=%s PawnNetMode=%s MovementNetMode=%s bClientContext=%s"),
                *PawnDescription,
                MO56RoleToString(LocalRole),
                MO56RoleToString(ObservedRemoteRole),
                bLocallyControlled ? TEXT("true") : TEXT("false"),
                MO56NetModeToString(ControllerNetMode),
                MO56NetModeToString(WorldNetMode),
                MO56NetModeToString(PawnNetMode),
                MovementNetModeString,
                bClientContext ? TEXT("true") : TEXT("false")), TargetPawn);

        if (TargetPawn && !bGood)
        {
                UE_LOG(LogTemp, Warning, TEXT("[PC] ClientValidatePostPossess role unexpected. PawnRole=%s PawnLocal=%s ControllerNetMode=%s"),
                        MO56RoleToString(LocalRole),
                        bLocallyControlled ? TEXT("true") : TEXT("false"),
                        MO56NetModeToString(ControllerNetMode));
                LogDebugEvent(TEXT("ClientValidatePostPossessRequestFix"), FString::Printf(TEXT("Pawn=%s"), *PawnDescription), TargetPawn);
                ServerRequestPostPossessNetUpdate(TargetPawn);
        }
}

void AMO56PlayerController::ClientForceOpenPossessMenu_Implementation()
{
        OpenPossessMenu();
}

void AMO56PlayerController::ClientPostRestartValidate_Implementation()
{
        PostRestartRetryCount = 0;

        if (GetNetMode() != NM_Client)
        {
                return;
        }

        TWeakObjectPtr<AMO56PlayerController> WeakThis(this);
        GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis]()
        {
                if (AMO56PlayerController* Controller = WeakThis.Get())
                {
                        Controller->GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis]()
                        {
                                if (AMO56PlayerController* InnerController = WeakThis.Get())
                                {
                                        InnerController->EvaluatePostRestartState();
                                }
                        }));
                }
        }));
}

void AMO56PlayerController::EvaluatePostRestartState()
{
        APawn* LocalPawn = GetPawn();
        if (!LocalPawn)
        {
                UE_LOG(LogTemp, Warning, TEXT("[PC] PostRestart validation skipped - no pawn."));
                return;
        }

        const ENetRole LocalRole = LocalPawn->GetLocalRole();
        const bool bLocallyControlled = LocalPawn->IsLocallyControlled();
        const ENetMode ControllerNetMode = GetNetMode();
        const ENetMode WorldNetMode = GetWorld() ? GetWorld()->GetNetMode() : NM_Standalone;
        const ENetMode PawnNetMode = LocalPawn->GetNetMode();
        const bool bClientContext = (ControllerNetMode == NM_Client);

        const bool bGood = (bClientContext && LocalRole == ROLE_AutonomousProxy)
                || (!bClientContext && LocalRole == ROLE_Authority)
                || bLocallyControlled;

        UE_LOG(LogTemp, Log, TEXT("[PC] PostRestart check PawnRole=%s PawnLocal=%s ControllerNetMode=%s WorldNetMode=%s PawnNetMode=%s Retry=%d"),
                MO56RoleToString(LocalRole),
                bLocallyControlled ? TEXT("true") : TEXT("false"),
                MO56NetModeToString(ControllerNetMode),
                MO56NetModeToString(WorldNetMode),
                MO56NetModeToString(PawnNetMode),
                PostRestartRetryCount);

        if (!bGood)
        {
                if (PostRestartRetryCount < PostRestartRetryLimit)
                {
                        ++PostRestartRetryCount;
                        UE_LOG(LogTemp, Warning, TEXT("[PC] PostRestart not in expected role. Reissuing ClientRestart."));
                        ClientRestart(LocalPawn);
                        ReapplyEnhancedInputContexts();
                        EnsureGameInputLocal();

                        TWeakObjectPtr<AMO56PlayerController> WeakThis(this);
                        FTimerHandle RetryHandle;
                        GetWorldTimerManager().SetTimer(RetryHandle, FTimerDelegate::CreateLambda([WeakThis]()
                        {
                                if (AMO56PlayerController* Controller = WeakThis.Get())
                                {
                                        Controller->EvaluatePostRestartState();
                                }
                        }), 0.1f, false);
                }
                else
                {
                        UE_LOG(LogTemp, Error, TEXT("[PC] PostRestart role still unexpected after retries. PawnRole=%s PawnLocal=%s ControllerNetMode=%s"),
                                MO56RoleToString(LocalRole),
                                bLocallyControlled ? TEXT("true") : TEXT("false"),
                                MO56NetModeToString(ControllerNetMode));
                }

                return;
        }

        UE_LOG(LogTemp, Log, TEXT("[PC] PostRestart validation succeeded. PawnRole=%s PawnLocal=%s ControllerNetMode=%s"),
                MO56RoleToString(LocalRole),
                bLocallyControlled ? TEXT("true") : TEXT("false"),
                MO56NetModeToString(ControllerNetMode));
        PostRestartRetryCount = 0;
}

void AMO56PlayerController::ServerRequestPostPossessNetUpdate_Implementation(APawn* TargetPawn)
{
        if (!HasAuthority() || !TargetPawn || TargetPawn->GetController() != this)
        {
                return;
        }

        LogDebugEvent(TEXT("ServerPostPossessNetUpdate"), FString::Printf(TEXT("Pawn=%s"), *DescribePawnForDebug(TargetPawn)), TargetPawn);

        TargetPawn->ForceNetUpdate();
        ClientRestart(TargetPawn);
        ClientReapplyEnhancedInputContexts();
        ClientEnsureGameInput();
        ClientPostRestartValidate();
}

void AMO56PlayerController::HandleNewGameOnServer(const FString& LevelName)
{
        if (!HasAuthority())
        {
                return;
        }

        LogDebugEvent(TEXT("HandleNewGameOnServer"), FString::Printf(TEXT("Level=%s"), *LevelName));

        if (UMO56SaveSubsystem* SaveSubsystem = GetSaveSubsystem())
        {
                const FString DesiredLevel = LevelName.IsEmpty() ? TEXT("TestLevel") : LevelName;
                SaveSubsystem->StartNewGame(DesiredLevel);
        }
}

void AMO56PlayerController::HandleSaveGameOnServer(bool bAlsoExit)
{
        if (!HasAuthority())
        {
                return;
        }

        LogDebugEvent(TEXT("HandleSaveGameOnServer"), FString::Printf(TEXT("bAlsoExit=%s"), bAlsoExit ? TEXT("true") : TEXT("false")));

        if (UMO56SaveSubsystem* SaveSubsystem = GetSaveSubsystem())
        {
                SaveSubsystem->SaveCurrentGame();
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

        LogDebugEvent(TEXT("HandleLoadGameOnServer"), FString::Printf(TEXT("Slot=%s User=%d"), *SlotName, UserIndex));

        bool bLoaded = false;

        if (UMO56SaveSubsystem* SaveSubsystem = GetSaveSubsystem())
        {
                if (!SlotName.IsEmpty())
                {
                        bLoaded = SaveSubsystem->LoadGameBySlot(SlotName, UserIndex);
                }
                else
                {
                        if (UMO56SaveGame* CurrentSave = SaveSubsystem->GetCurrentSaveGame())
                        {
                                if (CurrentSave->SaveId.IsValid())
                                {
                                        bLoaded = SaveSubsystem->LoadSave(CurrentSave->SaveId);
                                }
                        }
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

        LogDebugEvent(TEXT("HandleLoadGameOnServerComplete"), FString::Printf(TEXT("bLoaded=%s"), bLoaded ? TEXT("true") : TEXT("false")));

        return bLoaded;
}

bool AMO56PlayerController::HandleLoadGameByIdOnServer(const FGuid& SaveId)
{
        if (!HasAuthority() || !SaveId.IsValid())
        {
                return false;
        }

        LogDebugEvent(TEXT("HandleLoadGameByIdOnServer"), FString::Printf(TEXT("SaveId=%s"), *SaveId.ToString(EGuidFormats::DigitsWithHyphens)));

        bool bLoaded = false;

        if (UMO56SaveSubsystem* SaveSubsystem = GetSaveSubsystem())
        {
                if (SaveSubsystem->DoesSaveExist(SaveId))
                {
                        bLoaded = SaveSubsystem->LoadSave(SaveId);
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

        LogDebugEvent(TEXT("HandleLoadGameByIdOnServerComplete"), FString::Printf(TEXT("bLoaded=%s"), bLoaded ? TEXT("true") : TEXT("false")));

        return bLoaded;
}

bool AMO56PlayerController::HandleCreateNewSaveSlot()
{
        if (!HasAuthority())
        {
                return false;
        }

        LogDebugEvent(TEXT("HandleCreateNewSaveSlot"), FString(TEXT("Begin")));

        if (UMO56SaveSubsystem* SaveSubsystem = GetSaveSubsystem())
        {
                const FSaveGameSummary Summary = SaveSubsystem->CreateNewSaveSlot();
                LogDebugEvent(TEXT("HandleCreateNewSaveSlotComplete"), FString::Printf(TEXT("Created=%s"), Summary.SlotName.IsEmpty() ? TEXT("false") : TEXT("true")));
                return !Summary.SlotName.IsEmpty();
        }

        LogDebugEvent(TEXT("HandleCreateNewSaveSlotFailed"), FString(TEXT("NoSubsystem")));
        return false;
}

void AMO56PlayerController::HandlePossessPawn(APawn* TargetPawn)
{
        if (!HasAuthority() || !TargetPawn)
        {
                return;
        }

        APawn* PreviousPawn = GetPawn();
        if (TargetPawn == PreviousPawn)
        {
                LogDebugEvent(TEXT("HandlePossessPawn"), FString::Printf(TEXT("NoOp Target=%s"), *DescribePawnForDebug(TargetPawn)), TargetPawn);
                return;
        }

        LogDebugEvent(TEXT("HandlePossessPawn"), FString::Printf(TEXT("From=%s To=%s"),
                *DescribePawnForDebug(PreviousPawn),
                *DescribePawnForDebug(TargetPawn)), TargetPawn);

        if (PreviousPawn && PreviousPawn->IsA<ASpectatorPawn>())
        {
                UnPossess();
        }

        if (AController* ExistingController = TargetPawn->GetController())
        {
                if (ExistingController != this)
                {
                        ExistingController->UnPossess();
                }
        }

        Possess(TargetPawn);

        if (AMO56Character* OldCharacter = Cast<AMO56Character>(PreviousPawn))
        {
                OldCharacter->CloseAllPlayerMenus();
        }

        if (AMO56Character* NewCharacter = Cast<AMO56Character>(TargetPawn))
        {
                NewCharacter->CloseAllPlayerMenus();
                SetLastContainerOwningCharacter(NewCharacter);
        }

        if (IsLocalController())
        {
                EnsureDefaultInputContexts();
                ApplyGameplayInputState();
        }

        ClientEnsureGameInput();

        LogDebugEvent(TEXT("HandlePossessPawnComplete"), FString::Printf(TEXT("Now=%s"), *DescribePawnForDebug(GetPawn())));
}

void AMO56PlayerController::HandleOpenPawnInventory(APawn* TargetPawn)
{
        if (!IsLocalController() || !TargetPawn)
        {
                return;
        }

        LogDebugEvent(TEXT("HandleOpenPawnInventory"), FString::Printf(TEXT("Target=%s"), *DescribePawnForDebug(TargetPawn)), TargetPawn);

        AMO56Character* OwningCharacter = Cast<AMO56Character>(GetCharacter());
        if (!OwningCharacter)
        {
                return;
        }

        SetLastContainerOwningCharacter(OwningCharacter);

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

        LogDebugEvent(TEXT("HandlePawnContext"), FString::Printf(TEXT("Target=%s Focus=%s"), *DescribePawnForDebug(TargetPawn), bHasFocus ? TEXT("true") : TEXT("false")), TargetPawn);

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

        LogDebugEvent(TEXT("SetPlayerSaveId"), FString::Printf(TEXT("PlayerId=%s"), *InId.ToString(EGuidFormats::DigitsWithHyphens)));
}

void AMO56PlayerController::EnsureDefaultInputContexts()
{
        if (!IsLocalPlayerController())
        {
                return;
        }

        for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
        {
                ApplyMappingContext(CurrentContext, 0);
        }

        if (!SVirtualJoystick::ShouldDisplayTouchInterface())
        {
                for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
                {
                        ApplyMappingContext(CurrentContext, 0);
                }
        }
}

void AMO56PlayerController::ReapplyEnhancedInputContexts()
{
        if (!IsLocalPlayerController())
        {
                return;
        }

        if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
        {
                if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
                {
                        Subsystem->ClearAllMappings();

                        for (int32 Index = TrackedInputContexts.Num() - 1; Index >= 0; --Index)
                        {
                                const UInputMappingContext* Context = TrackedInputContexts[Index].Context.Get();
                                const int32 Priority = TrackedInputContexts[Index].Priority;

                                if (!Context)
                                {
                                        TrackedInputContexts.RemoveAt(Index);
                                        continue;
                                }

                                Subsystem->AddMappingContext(Context, Priority);
                        }
                }
        }
}

void AMO56PlayerController::ApplyMappingContext(const UInputMappingContext* Context, int32 Priority)
{
        if (!Context)
        {
                return;
        }

        bool bUpdated = false;
        for (FTrackedInputContext& Entry : TrackedInputContexts)
        {
                if (Entry.Context.Get() == Context)
                {
                        Entry.Context = Context;
                        Entry.Priority = Priority;
                        bUpdated = true;
                        break;
                }
        }

        if (!bUpdated)
        {
                FTrackedInputContext Add;
                Add.Context = Context;
                Add.Priority = Priority;
                TrackedInputContexts.Add(Add);
        }

        if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
        {
                if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
                {
                        Subsystem->AddMappingContext(Context, Priority);
                }
        }
}

void AMO56PlayerController::EnsureGameInputLocal()
{
        if (AMO56Character* MOCharacter = Cast<AMO56Character>(GetCharacter()))
        {
                MOCharacter->CloseAllPlayerMenus();
        }

        ApplyGameplayInputState();
}

void AMO56PlayerController::EnsureGameplayInputMode()
{
        EnsureDefaultInputContexts();
        ApplyGameplayInputState();
}

void AMO56PlayerController::ApplyGameplayInputState()
{
        FInputModeGameOnly InputMode;
        SetInputMode(InputMode);

        bShowMouseCursor = false;
        SetShowMouseCursor(false);
        bEnableClickEvents = false;
        bEnableMouseOverEvents = false;
        SetIgnoreLookInput(false);
        SetIgnoreMoveInput(false);

        MarkDebugInputMode(TEXT("GameOnly"));
        UE_LOG(LogTemp, Log, TEXT("[InputMode] Gameplay Cursor=%s Click=%s Over=%s IgnoreLook=%s IgnoreMove=%s"),
                bShowMouseCursor ? TEXT("true") : TEXT("false"),
                bEnableClickEvents ? TEXT("true") : TEXT("false"),
                bEnableMouseOverEvents ? TEXT("true") : TEXT("false"),
                IsLookInputIgnored() ? TEXT("true") : TEXT("false"),
                IsMoveInputIgnored() ? TEXT("true") : TEXT("false"));
}

void AMO56PlayerController::ApplyMenuInputState()
{
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
        bShowMouseCursor = true;
        SetShowMouseCursor(true);
        bEnableClickEvents = true;
        bEnableMouseOverEvents = true;
        SetIgnoreLookInput(true);
        SetIgnoreMoveInput(true);
        MarkDebugInputMode(TEXT("Menu"));
        UE_LOG(LogTemp, Log, TEXT("[InputMode] Menu Cursor=%s Click=%s Over=%s IgnoreLook=%s IgnoreMove=%s"),
                bShowMouseCursor ? TEXT("true") : TEXT("false"),
                bEnableClickEvents ? TEXT("true") : TEXT("false"),
                bEnableMouseOverEvents ? TEXT("true") : TEXT("false"),
                IsLookInputIgnored() ? TEXT("true") : TEXT("false"),
                IsMoveInputIgnored() ? TEXT("true") : TEXT("false"));
}

FString AMO56PlayerController::BuildInputStateSnapshot() const
{
        return DescribeInputState(*this);
}

void AMO56PlayerController::MarkDebugInputMode(FName ModeTag)
{
        DebugInputModeTag = ModeTag;
}

FString AMO56PlayerController::DescribeDebugInputMode() const
{
        return DebugInputModeTag.IsNone() ? FString(TEXT("Unknown")) : DebugInputModeTag.ToString();
}

void AMO56PlayerController::LogDebugEvent(FName Action, const FString& Detail, const APawn* ContextPawn) const
{
        if (UMO56DebugLogSubsystem* Subsystem = UMO56DebugLogSubsystem::Get(this))
        {
                if (Subsystem->IsEnabled())
                {
                        const FGuid PlayerGuid = ResolvePlayerGuid();
                        const APawn* PawnForEvent = ContextPawn ? ContextPawn : GetPawn();
                        const FGuid PawnGuid = PawnForEvent ? MO56ResolvePawnId(PawnForEvent) : FGuid();
                        Subsystem->LogEvent(TEXT("PlayerController"), Action, Detail, PlayerGuid, PawnGuid);
                }
        }
}

FGuid AMO56PlayerController::ResolvePlayerGuid() const
{
        if (PlayerSaveId.IsValid())
        {
                return PlayerSaveId;
        }

        return FGuid();
}

void AMO56PlayerController::SetLastContainerOwningCharacter(AMO56Character* InCharacter)
{
        LastContainerOwningCharacter = InCharacter;
}
