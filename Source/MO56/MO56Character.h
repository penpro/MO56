// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MO56Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
class UInventoryComponent; // forward declare
class UHUDWidget;
class UInventoryWidget;
class UCharacterStatusComponent;
class UCharacterStatusWidget;
class AActor;
class AInventoryContainer;
class UGameMenuWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AMO56Character : public ACharacter
{
        GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
        UInventoryComponent* Inventory;

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
        UCharacterStatusComponent* CharacterStatus;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

        /** Mouse Look Input Action */
        UPROPERTY(EditAnywhere, Category="Input")
        UInputAction* MouseLookAction;


public:

        /** Constructor */
        AMO56Character();

protected:

        /** Called when the game starts or when spawned */
        virtual void BeginPlay() override;
        virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
        virtual void Tick(float DeltaSeconds) override;

        UFUNCTION()
        void HandleInventoryUpdated();

        UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
        void OnInventoryUpdated();

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

        

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

        /** Called for interaction input */
        void OnInteract(const FInputActionValue& Value);

        /** Called for toggling the inventory UI */
        void OnToggleInventory(const FInputActionValue& Value);

        /** Called for toggling the character status UI */
        void OnToggleCharacterStatus(const FInputActionValue& Value);
        void OnToggleGameMenu(const FInputActionValue& Value);

        void SetInventoryVisible(bool bVisible);
        void SetCharacterStatusVisible(bool bVisible);
        void SetGameMenuVisible(bool bVisible);
        void UpdateInventoryInputState(bool bInventoryVisible);
        bool IsAnyInventoryPanelVisible() const;
        void CloseActiveContainerInventory(bool bNotifyContainer);

protected:
        UFUNCTION(Server, Reliable)
        void Server_Interact(AActor* HitActor);

public:
        void OpenContainerInventory(UInventoryComponent* ContainerInventory, AActor* ContainerActor);
        void CloseContainerInventoryForActor(AActor* ContainerActor, bool bClosePlayerInventory = true);

public:

        /** Interact Input Action */
        UPROPERTY(EditAnywhere, Category = "Input|Actions")
        UInputAction* InteractAction = nullptr;

        /** Inventory Input Action */
        UPROPERTY(EditAnywhere, Category = "Input|Actions")
        UInputAction* InventoryAction = nullptr;

        /** Character status Input Action */
        UPROPERTY(EditAnywhere, Category = "Input|Actions")
        UInputAction* CharacterStatusAction = nullptr;

        /** Game menu Input Action */
        UPROPERTY(EditAnywhere, Category = "Input|Actions")
        UInputAction* MenuAction = nullptr;

        /** Preferred game menu input action (IA_GameMenu). */
        UPROPERTY(EditAnywhere, Category = "Input|Actions")
        UInputAction* GameMenuAction = nullptr;

        /** Widget class for HUD */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
        TSubclassOf<class UHUDWidget> HUDWidgetClass;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
        TSubclassOf<class UCharacterStatusWidget> CharacterStatusWidgetClass;

        /** Widget class for the inventory display */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
        TSubclassOf<class UInventoryWidget> InventoryWidgetClass;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
        TSubclassOf<class UGameMenuWidget> GameMenuWidgetClass;

        /** Instance of the HUD widget */
        UPROPERTY(Transient)
        TObjectPtr<class UHUDWidget> HUDWidgetInstance;

        UPROPERTY(Transient)
        TObjectPtr<class UCharacterStatusWidget> CharacterStatusWidgetInstance;

        /** Instance of the inventory widget */
        UPROPERTY(Transient)
        TObjectPtr<class UInventoryWidget> InventoryWidgetInstance;

        /** Instance of the container inventory widget */
        UPROPERTY(Transient)
        TObjectPtr<class UInventoryWidget> ContainerInventoryWidgetInstance;

        UPROPERTY(Transient)
        TObjectPtr<class UGameMenuWidget> GameMenuWidgetInstance;

        /** Inventory currently displayed in the container panel */
        TWeakObjectPtr<UInventoryComponent> ActiveContainerInventory;

        /** Actor that provided the active container inventory */
        TWeakObjectPtr<AActor> ActiveContainerActor;

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

