// Copyright Epic Games, Inc. All Rights Reserved.

// Implementation: This character wires input, inventory, status, and skill menus for both
// single-player and multiplayer. Derive a blueprint, assign widget classes, and use the
// provided replicated properties to drive AI enable/disable states when players take over
// different pawns.
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
class USkillSystemComponent;
class UCraftingSystemComponent;
class UCharacterSkillMenu;
class UInspectionCountdownWidget;
class UWorldActorContextMenuWidget;
class APawn;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 *
 *  Editor Implementation Guide:
 *  1. Create a Blueprint subclass (e.g. BP_PlayerCharacter) and assign it as the default pawn in your GameMode.
 *  2. Configure CameraBoom/FollowCamera offsets, FOV, and collision probing to match the desired third-person view.
 *  3. Populate the Input|Actions properties with Enhanced Input assets, then add the mapping context in BeginPlay.
 *  4. Set HUDWidgetClass, InventoryWidgetClass, CharacterStatusWidgetClass, and Skill menu classes to hook UI prefabs.
 *  5. Wire Blueprint events (OnInventoryUpdated, OnToggleInventory, etc.) to show/hide widgets and coordinate containers.
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

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skills", meta = (AllowPrivateAccess = "true"))
        USkillSystemComponent* SkillSystem;

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting", meta = (AllowPrivateAccess = "true"))
        UCraftingSystemComponent* CraftingSystem;
	
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
        virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
        virtual void PossessedBy(AController* NewController) override;
        virtual void UnPossessed() override;

        UFUNCTION()
        void HandleInventoryUpdated();

        UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
        void OnInventoryUpdated();

        UFUNCTION()
        void OnRep_IsPossessed();

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
        void OnToggleSkillMenu(const FInputActionValue& Value);

        void SetInventoryVisible(bool bVisible);
        void SetCharacterStatusVisible(bool bVisible);
        void SetGameMenuVisible(bool bVisible);
        void SetSkillMenuVisible(bool bVisible);
        void UpdateInventoryInputState(bool bInventoryVisible);
        bool IsAnyInventoryPanelVisible() const;
        void CloseActiveContainerInventory(bool bNotifyContainer);
        void CloseWorldContextMenu();
        bool TryOpenWorldContextMenu();
        void HandleWorldContextMenuDismissed();

protected:
        UFUNCTION(Server, Reliable)
        void Server_Interact(AActor* HitActor);

public:
        void OpenContainerInventory(UInventoryComponent* ContainerInventory, AActor* ContainerActor);
        void CloseContainerInventoryForActor(AActor* ContainerActor, bool bClosePlayerInventory = true);

        /** Collapse every UI panel that the character manages (inventory, status, skills, menu, world context). */
        void CloseAllPlayerMenus();

        UFUNCTION(BlueprintPure, Category = "Inventory")
        UInventoryComponent* GetInventoryComponent() const { return Inventory; }

        UFUNCTION(BlueprintPure, Category = "Skills")
        USkillSystemComponent* GetSkillSystemComponent() const { return SkillSystem; }

        UFUNCTION(BlueprintPure, Category = "Crafting")
        UCraftingSystemComponent* GetCraftingSystemComponent() const { return CraftingSystem; }

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

        UPROPERTY(EditAnywhere, Category = "Input|Actions")
        UInputAction* SkillAction = nullptr;

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

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
        TSubclassOf<class UCharacterSkillMenu> CharacterSkillMenuClass;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
        TSubclassOf<class UInspectionCountdownWidget> InspectionCountdownWidgetClass;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
        TSubclassOf<class UWorldActorContextMenuWidget> WorldContextMenuClass;

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

        UPROPERTY(Transient)
        TObjectPtr<UWorldActorContextMenuWidget> ActiveWorldContextMenu;
        TWeakObjectPtr<APawn> FocusedContextPawn;
        bool bContextFocusActive = false;

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

        UFUNCTION(BlueprintCallable, Category = "Control")
        void SetEnableAI(bool bEnable);

        UFUNCTION(BlueprintPure, Category = "Control")
        bool IsAIEnabled() const { return bEnableAI; }

        UFUNCTION(BlueprintPure, Category = "Control")
        bool IsPossessedByPlayer() const { return bIsPossessed; }

protected:
        UPROPERTY(ReplicatedUsing = OnRep_IsPossessed, BlueprintReadOnly, Category = "Control")
        bool bIsPossessed = false;

        UPROPERTY(Replicated, BlueprintReadOnly, Category = "Control")
        bool bEnableAI = true;
};

