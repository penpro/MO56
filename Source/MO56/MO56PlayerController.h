// Implementation: Use this controller for both host and clients. Assign input mappings in
// the blueprint defaults, then bind UI widgets to the BlueprintCallable helpers so server
// authoritative actions (saving, possession, inventory interactions) are correctly routed.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Templates/Pair.h"
#include "MO56PlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UMO56SaveSubsystem;
class AInventoryContainer;
class UMO56PossessMenuWidget;
class UMO56PossessionMenuManagerSubsystem;
class UMO56DebugLogSubsystem;
class AMO56Character;

USTRUCT(BlueprintType)
struct FMOPossessablePawnInfo
{
        GENERATED_BODY()

        UPROPERTY(BlueprintReadOnly)
        FGuid PawnId;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FText DisplayName;

        UPROPERTY(BlueprintReadOnly)
        FVector Location = FVector::ZeroVector;

        UPROPERTY(BlueprintReadOnly)
        bool bAssigned = false;

        UPROPERTY(BlueprintReadOnly)
        FGuid AssignedTo;
};

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 *
 *  Editor Implementation Guide:
 *  1. Create a Blueprint subclass and assign it in Project Settings > Maps & Modes > Player Controller Class.
 *  2. Populate DefaultMappingContexts with Enhanced Input mapping assets; add/remove mobile-excluded contexts as needed.
 *  3. Set MobileControlsWidgetClass to a touch-specific widget and drive visibility via RequestOpenPawnInventory/Notify focus.
 *  4. Implement the BlueprintCallable Request* functions to trigger menu widgets, save prompts, or possession flows.
 *  5. Use replicated PlayerSaveId and save subsystem references to coordinate load/save UI from Blueprint events.
 */
UCLASS(abstract)
class AMO56PlayerController : public APlayerController
{
GENERATED_BODY()

friend class AInventoryContainer;
friend class UMO56SaveSubsystem;
friend class AMO56Character;
friend class UMO56PossessionMenuManagerSubsystem;

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;

/** Gameplay initialization */
virtual void BeginPlay() override;

virtual void OnPossess(APawn* InPawn) override;
virtual void OnUnPossess() override;
virtual void ClientRestart_Implementation(APawn* NewPawn) override;

/** Input mapping context setup */
virtual void SetupInputComponent() override;

virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
        UFUNCTION(BlueprintCallable, Category = "Game|Flow")
        void RequestNewGame();

        UFUNCTION(BlueprintCallable, Category = "Game|Flow")
        void RequestSaveGame();

        UFUNCTION(BlueprintCallable, Category = "Game|Flow")
        void RequestSaveAndExit();

        UFUNCTION(BlueprintCallable, Category = "Game|Flow")
        bool RequestLoadGame();

        UFUNCTION(BlueprintCallable, Category = "Game|Flow")
        bool RequestLoadGameBySlot(const FString& SlotName, int32 UserIndex);

        UFUNCTION(BlueprintCallable, Category = "Game|Flow")
        bool RequestLoadGameById(const FGuid& SaveId);

        UFUNCTION(BlueprintCallable, Category = "Game|Flow")
        bool RequestCreateNewSaveSlot();

        UFUNCTION(BlueprintCallable, Category = "Control")
        void RequestPossessPawn(APawn* TargetPawn);

        UFUNCTION(Server, Reliable)
        void ServerQueryPossessablePawns();

        UFUNCTION(Client, Reliable)
        void ClientReceivePossessablePawns(const TArray<FMOPossessablePawnInfo>& List);

        UFUNCTION(Server, Reliable)
        void ServerRequestPossessPawnById(const FGuid& PawnId);

        void OpenPossessMenu();
        void ClosePossessMenu();

        TSubclassOf<UMO56PossessMenuWidget> GetPossessMenuClass() const { return PossessMenuClass; }
        const TArray<FMOPossessablePawnInfo>& GetCachedPossessablePawns() const { return CachedPossessablePawns; }

UFUNCTION(BlueprintCallable, Category = "Control")
void RequestOpenPawnInventory(APawn* TargetPawn);

UFUNCTION(BlueprintCallable, Category = "Control")
void NotifyPawnContextFocus(APawn* TargetPawn, bool bHasFocus);

UFUNCTION(BlueprintCallable, Category = "Inventory")
void RequestContainerInventoryOwnership(AInventoryContainer* ContainerActor);

UFUNCTION(BlueprintCallable, Category = "Inventory")
void NotifyContainerInventoryClosed(AInventoryContainer* ContainerActor);

FGuid GetPlayerSaveId() const { return PlayerSaveId; }

        FString DescribeDebugInputMode() const;

        const TArray<TPair<TWeakObjectPtr<const UInputMappingContext>, int32>>& GetTrackedActiveContexts() const { return TrackedInputContexts; }

protected:
        UFUNCTION(Server, Reliable)
        void ServerExecuteNewGame(const FString& LevelName);

        UFUNCTION(Server, Reliable)
        void ServerSaveGame();

        UFUNCTION(Server, Reliable)
        void ServerSaveAndExit();

        UFUNCTION(Server, Reliable)
        void ServerLoadGame(const FString& SlotName, int32 UserIndex);

        UFUNCTION(Server, Reliable)
        void ServerLoadGameById(const FGuid& SaveId);

        UFUNCTION(Server, Reliable)
        void ServerCreateNewSaveSlot();

        UFUNCTION(Server, Reliable)
        void ServerPossessPawn(APawn* TargetPawn);

        UFUNCTION(Server, Reliable)
        void ServerOpenPawnInventory(APawn* TargetPawn);

        UFUNCTION(Server, Reliable)
        void ServerSetPawnContext(APawn* TargetPawn, bool bHasFocus);

UFUNCTION(Client, Reliable)
void ClientOpenPawnInventoryResponse(APawn* TargetPawn);

UFUNCTION(Client, Reliable)
void ClientOpenContainerInventory(AInventoryContainer* ContainerActor);

UFUNCTION(Client, Reliable)
void ClientCloseContainerInventory(AInventoryContainer* ContainerActor);

        UFUNCTION(Client, Reliable)
        void ClientEnsureGameInput();

        UFUNCTION(Client, Reliable)
        void ClientReapplyEnhancedInputContexts();

        UFUNCTION(Client, Reliable)
        void ClientPostRestartValidate();

        UFUNCTION(Client, Reliable)
        void ClientValidatePostPossess(APawn* TargetPawn);

        UFUNCTION(Server, Reliable)
        void ServerRequestPostPossessNetUpdate(APawn* TargetPawn);

        UFUNCTION(Client, Reliable)
        void ClientForceOpenPossessMenu();

UFUNCTION(Server, Reliable)
void ServerRequestContainerInventoryOwnership(AInventoryContainer* ContainerActor);

UFUNCTION(Server, Reliable)
void ServerNotifyContainerInventoryClosed(AInventoryContainer* ContainerActor);

private:
        void HandleNewGameOnServer(const FString& LevelName);
        void HandleSaveGameOnServer(bool bAlsoExit);
        bool HandleLoadGameOnServer(const FString& SlotName, int32 UserIndex);
        bool HandleLoadGameByIdOnServer(const FGuid& SaveId);
        bool HandleCreateNewSaveSlot();
        void HandlePossessPawn(APawn* TargetPawn);
        void HandleOpenPawnInventory(APawn* TargetPawn);
        void HandlePawnContext(APawn* TargetPawn, bool bHasFocus);
        UMO56SaveSubsystem* GetSaveSubsystem() const;

        void SetPlayerSaveId(const FGuid& InId);

        void EnsureDefaultInputContexts();
        void ReapplyEnhancedInputContexts();
        void ApplyMappingContext(const UInputMappingContext* Context, int32 Priority);
        void EnsureGameplayInputMode();
        void ApplyGameplayInputState();
        void ApplyMenuInputState();
        FString BuildInputStateSnapshot() const;
        void MarkDebugInputMode(FName ModeTag);
        void LogDebugEvent(FName Action, const FString& Detail, const APawn* ContextPawn = nullptr) const;
        FGuid ResolvePlayerGuid() const;
        void SetLastContainerOwningCharacter(AMO56Character* InCharacter);
        void EvaluatePostRestartState();

        UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
        TSubclassOf<UMO56PossessMenuWidget> PossessMenuClass;

        UPROPERTY()
        TArray<FMOPossessablePawnInfo> CachedPossessablePawns;

        void OnIA_Possession();

        UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Save", meta = (AllowPrivateAccess = "true"))
        FGuid PlayerSaveId;

        TWeakObjectPtr<AMO56Character> LastContainerOwningCharacter;

        FName DebugInputModeTag = NAME_None;

        UPROPERTY(Transient)
        TArray<TPair<TWeakObjectPtr<const UInputMappingContext>, int32>> TrackedInputContexts;

        uint8 PostRestartRetryCount = 0;
        static constexpr uint8 PostRestartRetryLimit = 2;
};
