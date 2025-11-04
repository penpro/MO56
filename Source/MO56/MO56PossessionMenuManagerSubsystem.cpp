#include "MO56PossessionMenuManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "CoreUObjectDelegates.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "MO56PlayerController.h"
#include "TimerManager.h"
#include "UI/MO56PossessMenuWidget.h"

namespace
{
constexpr int32 PossessionMappingPriority = 100;
static const TCHAR* PossessionActionPath = TEXT("/Game/Input/Actions/IA_Possession.IA_Possession");
static const TCHAR* PossessionContextPath = TEXT("/Game/Input/IMC_Possession.IMC_Possession");
}

void UMO56PossessionMenuManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ArmDeferredSetup();
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

    Super::Deinitialize();
}

void UMO56PossessionMenuManagerSubsystem::ToggleMenuForLocalPlayer(ULocalPlayer* LocalPlayer)
{
    if (!LocalPlayer)
    {
        return;
    }

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

    OpenMenu(LocalPlayer);
}

void UMO56PossessionMenuManagerSubsystem::EnsureMenuClosedForLocalPlayer(ULocalPlayer* LocalPlayer)
{
    if (!LocalPlayer || !IsMenuOpenForLocalPlayer(LocalPlayer))
    {
        return;
    }

    CloseMenu(LocalPlayer);
}

void UMO56PossessionMenuManagerSubsystem::RefreshMenuForLocalPlayer(ULocalPlayer* LocalPlayer, const TArray<FMOPossessablePawnInfo>& PawnInfos)
{
    if (!LocalPlayer)
    {
        return;
    }

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
        RestoreInput(PlayerController, *State);
    }

    PerPlayerState.Remove(LocalPlayer);
}

void UMO56PossessionMenuManagerSubsystem::ApplyUIOnly(APlayerController* PlayerController, UUserWidget* RootWidget)
{
    if (!PlayerController || !RootWidget)
    {
        return;
    }

    PlayerController->bShowMouseCursor = true;
    PlayerController->bEnableClickEvents = true;
    PlayerController->bEnableMouseOverEvents = true;

    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController, RootWidget, EMouseLockMode::DoNotLock, true);

    PlayerController->SetIgnoreLookInput(true);
    PlayerController->SetIgnoreMoveInput(true);
}

void UMO56PossessionMenuManagerSubsystem::RestoreInput(APlayerController* PlayerController, const FMO56MenuState& State)
{
    if (!PlayerController)
    {
        return;
    }

    PlayerController->SetIgnoreLookInput(State.bHadInputModeSaved ? State.bPrevIgnoreLookInput : false);
    PlayerController->SetIgnoreMoveInput(State.bHadInputModeSaved ? State.bPrevIgnoreMoveInput : false);

    PlayerController->bShowMouseCursor = State.bHadInputModeSaved ? State.bPrevShowMouseCursor : false;
    PlayerController->bEnableClickEvents = State.bHadInputModeSaved ? State.bPrevEnableClick : false;
    PlayerController->bEnableMouseOverEvents = State.bHadInputModeSaved ? State.bPrevEnableMouseOver : false;

    FInputModeGameOnly GameOnly;
    PlayerController->SetInputMode(GameOnly);
}
