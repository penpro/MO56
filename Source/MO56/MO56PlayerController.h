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

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AMO56PlayerController : public APlayerController
{
	GENERATED_BODY()
	
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

public:
        UFUNCTION(BlueprintCallable, Category = "Game|Flow")
        void RequestNewGame();

        UFUNCTION(BlueprintCallable, Category = "Game|Flow")
        void RequestSaveGame();

        UFUNCTION(BlueprintCallable, Category = "Game|Flow")
        void RequestSaveAndExit();

        UFUNCTION(BlueprintCallable, Category = "Control")
        void RequestPossessPawn(APawn* TargetPawn);

        UFUNCTION(BlueprintCallable, Category = "Control")
        void RequestOpenPawnInventory(APawn* TargetPawn);

        UFUNCTION(BlueprintCallable, Category = "Control")
        void NotifyPawnContextFocus(APawn* TargetPawn, bool bHasFocus);

protected:
        UFUNCTION(Server, Reliable)
        void ServerExecuteNewGame(const FString& LevelName);

        UFUNCTION(Server, Reliable)
        void ServerSaveGame();

        UFUNCTION(Server, Reliable)
        void ServerSaveAndExit();

        UFUNCTION(Server, Reliable)
        void ServerPossessPawn(APawn* TargetPawn);

        UFUNCTION(Server, Reliable)
        void ServerOpenPawnInventory(APawn* TargetPawn);

        UFUNCTION(Server, Reliable)
        void ServerSetPawnContext(APawn* TargetPawn, bool bHasFocus);

        UFUNCTION(Client, Reliable)
        void ClientOpenPawnInventoryResponse(APawn* TargetPawn);

private:
        void HandleNewGameOnServer(const FString& LevelName);
        void HandleSaveGameOnServer(bool bAlsoExit);
        void HandlePossessPawn(APawn* TargetPawn);
        void HandleOpenPawnInventory(APawn* TargetPawn);
        void HandlePawnContext(APawn* TargetPawn, bool bHasFocus);
        UMO56SaveSubsystem* GetSaveSubsystem() const;
};
