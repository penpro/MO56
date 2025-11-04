#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "MO56PossessionMenuManagerSubsystem.generated.h"

#include "Containers/Array.h"
#include "Containers/Map.h"

class UInputAction;
class UInputMappingContext;
class UMO56PossessMenuWidget;
class UUserWidget;
class ULocalPlayer;
class APlayerController;

struct FMOPossessablePawnInfo;

USTRUCT()
struct FMO56MenuState
{
    GENERATED_BODY()

    TWeakObjectPtr<UUserWidget> Menu;
    bool bPrevShowMouseCursor = false;
    bool bPrevEnableClick = false;
    bool bPrevEnableMouseOver = false;
    bool bPrevIgnoreLookInput = false;
    bool bPrevIgnoreMoveInput = false;
    bool bHadInputModeSaved = false;
};

UCLASS()
class MO56_API UMO56PossessionMenuManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void ToggleMenuForLocalPlayer(ULocalPlayer* LocalPlayer);

    void EnsureBindingsForAllLocalPlayers();
    void EnsureMenuOpenForLocalPlayer(ULocalPlayer* LocalPlayer);
    void EnsureMenuClosedForLocalPlayer(ULocalPlayer* LocalPlayer);
    void RefreshMenuForLocalPlayer(ULocalPlayer* LocalPlayer, const TArray<FMOPossessablePawnInfo>& PawnInfos);
    bool IsMenuOpenForLocalPlayer(ULocalPlayer* LocalPlayer) const;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    void ArmDeferredSetup();
    void EnsureAssetsLoaded();

    void OpenMenu(ULocalPlayer* LocalPlayer);
    void CloseMenu(ULocalPlayer* LocalPlayer);
    void ApplyUIOnly(APlayerController* PlayerController, UUserWidget* RootWidget);
    void RestoreInput(APlayerController* PlayerController, const FMO56MenuState& State);

    bool CanCreateMenuForPlayer(ULocalPlayer* LocalPlayer) const;

    TMap<TWeakObjectPtr<ULocalPlayer>, FMO56MenuState> PerPlayerState;
    FDelegateHandle PostLoadHandle;

    UPROPERTY()
    TObjectPtr<UInputAction> PossessionAction = nullptr;

    UPROPERTY()
    TObjectPtr<UInputMappingContext> PossessionContext = nullptr;
};
