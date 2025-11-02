// Implementation: Use this controller for both host and clients. Assign input mappings in
// the blueprint defaults, then bind UI widgets to the BlueprintCallable helpers so server
// authoritative actions (saving, possession, inventory interactions) are correctly routed.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MO56PlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UMO56SaveSubsystem;
class AInventoryContainer;

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
        bool RequestCreateNewSaveSlot();

        UFUNCTION(BlueprintCallable, Category = "Control")
        void RequestPossessPawn(APawn* TargetPawn);

UFUNCTION(BlueprintCallable, Category = "Control")
void RequestOpenPawnInventory(APawn* TargetPawn);

UFUNCTION(BlueprintCallable, Category = "Control")
void NotifyPawnContextFocus(APawn* TargetPawn, bool bHasFocus);

UFUNCTION(BlueprintCallable, Category = "Inventory")
void RequestContainerInventoryOwnership(AInventoryContainer* ContainerActor);

UFUNCTION(BlueprintCallable, Category = "Inventory")
void NotifyContainerInventoryClosed(AInventoryContainer* ContainerActor);

FGuid GetPlayerSaveId() const { return PlayerSaveId; }

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

UFUNCTION(Server, Reliable)
void ServerRequestContainerInventoryOwnership(AInventoryContainer* ContainerActor);

UFUNCTION(Server, Reliable)
void ServerNotifyContainerInventoryClosed(AInventoryContainer* ContainerActor);

private:
        void HandleNewGameOnServer(const FString& LevelName);
        void HandleSaveGameOnServer(bool bAlsoExit);
        bool HandleLoadGameOnServer(const FString& SlotName, int32 UserIndex);
        bool HandleCreateNewSaveSlot();
        void HandlePossessPawn(APawn* TargetPawn);
        void HandleOpenPawnInventory(APawn* TargetPawn);
        void HandlePawnContext(APawn* TargetPawn, bool bHasFocus);
        UMO56SaveSubsystem* GetSaveSubsystem() const;

        void SetPlayerSaveId(const FGuid& InId);

        UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Save", meta = (AllowPrivateAccess = "true"))
        FGuid PlayerSaveId;
};
