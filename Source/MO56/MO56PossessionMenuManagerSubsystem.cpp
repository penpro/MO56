#include "MO56PossessionMenuManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "MO56PlayerController.h"
#include "MO56DebugLogSubsystem.h"
#include "TimerManager.h"
#include "UI/MO56PossessMenuWidget.h"

namespace
{
constexpr int32 PossessionMappingPriority = 100;
static const TCHAR* PossessionActionPath = TEXT("/Game/Input/Actions/IA_Possession.IA_Possession");
static const TCHAR* PossessionContextPath = TEXT("/Game/Input/IMC_Possession.IMC_Possession");
}

namespace
{
    FString DescribeControllerInput(const APlayerController* PlayerController)
    {
        if (!PlayerController)
        {
            return TEXT("Input=None");
        }

        return FString::Printf(TEXT("Input(Mouse=%s Click=%s Over=%s IgnoreLook=%s IgnoreMove=%s)"),
            PlayerController->bShowMouseCursor ? TEXT("true") : TEXT("false"),
            PlayerController->bEnableClickEvents ? TEXT("true") : TEXT("false"),
            PlayerController->bEnableMouseOverEvents ? TEXT("true") : TEXT("false"),
            PlayerController->IsLookInputIgnored() ? TEXT("true") : TEXT("false"),
            PlayerController->IsMoveInputIgnored() ? TEXT("true") : TEXT("false"));
    }

    void LogMenuDebug(const UObject* Context, FName Action, ULocalPlayer* LocalPlayer, const FString& Detail, APlayerController* PlayerController)
    {
        if (UMO56DebugLogSubsystem* Subsystem = UMO56DebugLogSubsystem::Get(Context))
        {
            if (Subsystem->IsEnabled())
            {
                FGuid PlayerId;
                FGuid PawnId;

                if (const AMO56PlayerController* MOController = Cast<AMO56PlayerController>(PlayerController))
                {
                    PlayerId = MOController->GetPlayerSaveId();
                    PawnId = MO56ResolvePawnId(MOController->GetPawn());
                }

                Subsystem->LogEvent(TEXT("PossessionMenu"), Action, Detail, PlayerId, PawnId);
            }
        }
    }
}

void UMO56PossessionMenuManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ArmDeferredSetup();

    LogMenuDebug(this, TEXT("Initialize"), nullptr, TEXT("Possession menu subsystem initialized"), nullptr);
}

void UMO56PossessionMenuManagerSubsystem::Deinitialize()
{
    if (PostLoadHandle.IsValid())
    {
        FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadHandle);
        PostLoadHandle.Reset();
    }

    PerPlayerState.Empty();
    PossessionContext = nullptr;
    PossessionAction = nullptr;

    LogMenuDebug(this, TEXT("Deinitialize"), nullptr, TEXT("Possession menu subsystem shutdown"), nullptr);

    Super::Deinitialize();
}

void UMO56PossessionMenuManagerSubsystem::ToggleMenuForLocalPlayer(ULocalPlayer* LocalPlayer)
{
    if (!LocalPlayer)
    {
        return;
    }

    APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld());
    const bool bCurrentlyOpen = IsMenuOpenForLocalPlayer(LocalPlayer);
    LogMenuDebug(this, TEXT("ToggleMenu"), LocalPlayer, FString::Printf(TEXT("CurrentlyOpen=%s"), bCurrentlyOpen ? TEXT("true") : TEXT("false")), PlayerController);

    if (IsMenuOpenForLocalPlayer(LocalPlayer))
    {
        CloseMenu(LocalPlayer);
    }
    else
    {
        OpenMenu(LocalPlayer);
    }
}

void UMO56PossessionMenuManagerSubsystem::EnsureBindingsForAllLocalPlayers()
{
    EnsureAssetsLoaded();

    if (!PossessionContext)
    {
        return;
    }

    LogMenuDebug(this, TEXT("EnsureBindings"), nullptr, TEXT("Ensuring possession menu bindings"), nullptr);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        // Clean up stale entries
        for (auto It = PerPlayerState.CreateIterator(); It; ++It)
        {
            if (!It.Key().IsValid() || !It.Value().Menu.IsValid())
            {
                It.RemoveCurrent();
            }
        }

        for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
        {
            if (!LocalPlayer)
            {
                continue;
            }

            if (UEnhancedInputLocalPlayerSubsystem* EnhancedSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                EnhancedSubsystem->AddMappingContext(PossessionContext, PossessionMappingPriority);

                APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld());
                LogMenuDebug(this, TEXT("AddMappingContext"), LocalPlayer, FString::Printf(TEXT("Context=%s Priority=%d"), *GetNameSafe(PossessionContext), PossessionMappingPriority), PlayerController);
            }
        }
    }
}

void UMO56PossessionMenuManagerSubsystem::EnsureMenuOpenForLocalPlayer(ULocalPlayer* LocalPlayer)
{
    if (!LocalPlayer || IsMenuOpenForLocalPlayer(LocalPlayer))
    {
        return;
    }

    APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld());
    LogMenuDebug(this, TEXT("EnsureMenuOpen"), LocalPlayer, TEXT("Ensuring menu open"), PlayerController);

    OpenMenu(LocalPlayer);
}

void UMO56PossessionMenuManagerSubsystem::EnsureMenuClosedForLocalPlayer(ULocalPlayer* LocalPlayer)
{
    if (!LocalPlayer || !IsMenuOpenForLocalPlayer(LocalPlayer))
    {
        return;
    }

    APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld());
    LogMenuDebug(this, TEXT("EnsureMenuClosed"), LocalPlayer, TEXT("Ensuring menu closed"), PlayerController);

    CloseMenu(LocalPlayer);
}

void UMO56PossessionMenuManagerSubsystem::RefreshMenuForLocalPlayer(ULocalPlayer* LocalPlayer, const TArray<FMOPossessablePawnInfo>& PawnInfos)
{
    if (!LocalPlayer)
    {
        return;
    }

    APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld());
    LogMenuDebug(this, TEXT("RefreshMenu"), LocalPlayer, FString::Printf(TEXT("Entries=%d"), PawnInfos.Num()), PlayerController);

    if (FMO56MenuState* State = PerPlayerState.Find(LocalPlayer))
    {
        if (UMO56PossessMenuWidget* MenuWidget = Cast<UMO56PossessMenuWidget>(State->Menu.Get()))
        {
            MenuWidget->SetList(PawnInfos);
        }
    }
}

bool UMO56PossessionMenuManagerSubsystem::IsMenuOpenForLocalPlayer(ULocalPlayer* LocalPlayer) const
{
    if (!LocalPlayer)
    {
        return false;
    }

    if (const FMO56MenuState* State = PerPlayerState.Find(LocalPlayer))
    {
        return State->Menu.IsValid();
    }

    return false;
}

void UMO56PossessionMenuManagerSubsystem::ArmDeferredSetup()
{
    if (!PostLoadHandle.IsValid())
    {
        PostLoadHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddWeakLambda(this, [this](UWorld*)
        {
            EnsureBindingsForAllLocalPlayers();
        });
    }

    LogMenuDebug(this, TEXT("ArmDeferredSetup"), nullptr, TEXT("Arm deferred setup"), nullptr);

    EnsureBindingsForAllLocalPlayers();
}

void UMO56PossessionMenuManagerSubsystem::EnsureAssetsLoaded()
{
    if (!PossessionAction)
    {
        PossessionAction = LoadObject<UInputAction>(nullptr, PossessionActionPath);
        if (!PossessionAction)
        {
            PossessionAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Possession.IA_Possession"));
        }
    }

    if (!PossessionContext)
    {
        PossessionContext = LoadObject<UInputMappingContext>(nullptr, PossessionContextPath);
        if (!PossessionContext)
        {
            PossessionContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/Actions/IMC_Possession.IMC_Possession"));
        }

        if (!PossessionContext)
        {
            PossessionContext = NewObject<UInputMappingContext>(this);

            if (PossessionContext && PossessionAction)
            {
                PossessionContext->MapKey(PossessionAction, EKeys::P);
                PossessionContext->MapKey(PossessionAction, EKeys::Gamepad_Special_Right);
                PossessionContext->MapKey(PossessionAction, EKeys::Gamepad_FaceButton_Top);
            }
        }
    }

    LogMenuDebug(this, TEXT("EnsureAssetsLoaded"), nullptr, FString::Printf(TEXT("Action=%s Context=%s"), *GetNameSafe(PossessionAction), *GetNameSafe(PossessionContext)), nullptr);
}

bool UMO56PossessionMenuManagerSubsystem::CanCreateMenuForPlayer(ULocalPlayer* LocalPlayer) const
{
    if (!LocalPlayer)
    {
        return false;
    }

    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PlayerController = LocalPlayer->GetPlayerController(World))
        {
            return PlayerController->IsLocalPlayerController();
        }
    }

    return false;
}

void UMO56PossessionMenuManagerSubsystem::OpenMenu(ULocalPlayer* LocalPlayer)
{
    if (!CanCreateMenuForPlayer(LocalPlayer))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PlayerController = LocalPlayer->GetPlayerController(World);
    if (!PlayerController)
    {
        FTimerDelegate RetryDelegate;
        RetryDelegate.BindWeakLambda(this, [this, LocalPlayer]()
        {
            OpenMenu(LocalPlayer);
        });

        World->GetTimerManager().SetTimerForNextTick(RetryDelegate);
        return;
    }

    AMO56PlayerController* MO56Controller = Cast<AMO56PlayerController>(PlayerController);

    TSubclassOf<UMO56PossessMenuWidget> MenuClass = UMO56PossessMenuWidget::StaticClass();
    if (MO56Controller && MO56Controller->GetPossessMenuClass())
    {
        MenuClass = MO56Controller->GetPossessMenuClass();
    }

    const FString BeforeState = DescribeControllerInput(PlayerController);
    LogMenuDebug(this, TEXT("OpenMenu"), LocalPlayer, FString::Printf(TEXT("Before=%s"), *BeforeState), PlayerController);

    UMO56PossessMenuWidget* MenuWidget = CreateWidget<UMO56PossessMenuWidget>(PlayerController, MenuClass);
    if (!MenuWidget)
    {
        return;
    }

    MenuWidget->AddToViewport(10000);

    if (MO56Controller)
    {
        MenuWidget->SetList(MO56Controller->GetCachedPossessablePawns());
    }

    FMO56MenuState& State = PerPlayerState.FindOrAdd(LocalPlayer);
    State.Menu = MenuWidget;

    if (PlayerController)
    {
        State.bPrevShowMouseCursor = PlayerController->bShowMouseCursor;
        State.bPrevEnableClick = PlayerController->bEnableClickEvents;
        State.bPrevEnableMouseOver = PlayerController->bEnableMouseOverEvents;
        State.bPrevIgnoreLookInput = PlayerController->IsLookInputIgnored();
        State.bPrevIgnoreMoveInput = PlayerController->IsMoveInputIgnored();
        State.bHadInputModeSaved = true;
    }

    ApplyUIOnly(PlayerController, MenuWidget);

    LogMenuDebug(this, TEXT("OpenMenuApplied"), LocalPlayer, FString::Printf(TEXT("After=%s"), *DescribeControllerInput(PlayerController)), PlayerController);
}

void UMO56PossessionMenuManagerSubsystem::CloseMenu(ULocalPlayer* LocalPlayer)
{
    if (!LocalPlayer)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FMO56MenuState* State = PerPlayerState.Find(LocalPlayer);
    if (!State)
    {
        return;
    }

    if (UUserWidget* MenuWidget = State->Menu.Get())
    {
        MenuWidget->RemoveFromParent();
    }

    if (APlayerController* PlayerController = LocalPlayer->GetPlayerController(World))
    {
        LogMenuDebug(this, TEXT("CloseMenu"), LocalPlayer, FString::Printf(TEXT("Before=%s"), *DescribeControllerInput(PlayerController)), PlayerController);
        RestoreInput(PlayerController, *State);
        LogMenuDebug(this, TEXT("CloseMenuRestored"), LocalPlayer, FString::Printf(TEXT("After=%s"), *DescribeControllerInput(PlayerController)), PlayerController);
    }

    PerPlayerState.Remove(LocalPlayer);
}

void UMO56PossessionMenuManagerSubsystem::ApplyUIOnly(APlayerController* PlayerController, UUserWidget* RootWidget)
{
    if (!PlayerController || !RootWidget)
    {
        return;
    }

    LogMenuDebug(this, TEXT("ApplyUIOnly"), PlayerController ? PlayerController->GetLocalPlayer() : nullptr, FString::Printf(TEXT("Before=%s"), *DescribeControllerInput(PlayerController)), PlayerController);

    PlayerController->bShowMouseCursor = true;
    PlayerController->bEnableClickEvents = true;
    PlayerController->bEnableMouseOverEvents = true;

    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController, RootWidget, EMouseLockMode::DoNotLock, true);

    PlayerController->SetIgnoreLookInput(true);
    PlayerController->SetIgnoreMoveInput(true);

    LogMenuDebug(this, TEXT("ApplyUIOnlyComplete"), PlayerController ? PlayerController->GetLocalPlayer() : nullptr, FString::Printf(TEXT("After=%s"), *DescribeControllerInput(PlayerController)), PlayerController);
}

void UMO56PossessionMenuManagerSubsystem::RestoreInput(APlayerController* PlayerController, const FMO56MenuState& State)
{
    if (!PlayerController)
    {
        return;
    }

    LogMenuDebug(this, TEXT("RestoreInput"), PlayerController ? PlayerController->GetLocalPlayer() : nullptr, FString::Printf(TEXT("TargetPrevMouse=%s"), State.bPrevShowMouseCursor ? TEXT("true") : TEXT("false")), PlayerController);

    PlayerController->SetIgnoreLookInput(State.bHadInputModeSaved ? State.bPrevIgnoreLookInput : false);
    PlayerController->SetIgnoreMoveInput(State.bHadInputModeSaved ? State.bPrevIgnoreMoveInput : false);

    PlayerController->bShowMouseCursor = State.bHadInputModeSaved ? State.bPrevShowMouseCursor : false;
    PlayerController->bEnableClickEvents = State.bHadInputModeSaved ? State.bPrevEnableClick : false;
    PlayerController->bEnableMouseOverEvents = State.bHadInputModeSaved ? State.bPrevEnableMouseOver : false;

    FInputModeGameOnly GameOnly;
    PlayerController->SetInputMode(GameOnly);

    LogMenuDebug(this, TEXT("RestoreInputComplete"), PlayerController ? PlayerController->GetLocalPlayer() : nullptr, FString::Printf(TEXT("After=%s"), *DescribeControllerInput(PlayerController)), PlayerController);
}
