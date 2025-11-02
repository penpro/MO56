// Implementation: Handles input, UI widgets, inventory management, and multiplayer context
// menus. Blueprint subclasses can extend this behaviour and react to the replicated AI control
// flags when players take over different pawns.
#include "MO56Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Math/UnrealMathUtility.h"

#include "MO56.h"

#include "Interactable.h"
#include "InventoryContainer.h"

#include "DrawDebugHelpers.h"

#include "InventoryComponent.h" // from MOInventory
#include "Crafting/CraftingSystemComponent.h"
#include "MO56PlayerController.h"
#include "UI/HUDWidget.h"
#include "UI/InventoryUpdateInterface.h"
#include "UI/InventoryWidget.h"
#include "UI/GameMenuWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Components/CharacterStatusComponent.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/CharacterStatusWidget.h"
#include "Save/MO56SaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Skills/SkillSystemComponent.h"
#include "UI/CharacterSkillMenu.h"
#include "UI/WorldActorContextMenuWidget.h"
#include "Skills/InspectableComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Net/UnrealNetwork.h"
#include "MO56PlayerController.h"

AMO56Character::AMO56Character()
{
        // Set size for collision capsule
        GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

        Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
        CharacterStatus = CreateDefaultSubobject<UCharacterStatusComponent>(TEXT("CharacterStatus"));
        SkillSystem = CreateDefaultSubobject<USkillSystemComponent>(TEXT("SkillSystem"));
        CraftingSystem = CreateDefaultSubobject<UCraftingSystemComponent>(TEXT("CraftingSystem"));

        WorldContextMenuClass = UWorldActorContextMenuWidget::StaticClass();

        bReplicates = true;
        SetReplicateMovement(true);

        if (USkeletalMeshComponent* MeshComponent = GetMesh())
        {
                MeshComponent->SetIsReplicated(true);
        }
}


void AMO56Character::BeginPlay()
{
        Super::BeginPlay();

        if (HUDWidgetClass)
        {
                if (APlayerController* PC = Cast<APlayerController>(GetController()))
                {
                        HUDWidgetInstance = CreateWidget<UHUDWidget>(PC, HUDWidgetClass);
                }
                else
                {
                        HUDWidgetInstance = CreateWidget<UHUDWidget>(GetWorld(), HUDWidgetClass);
                }

                if (HUDWidgetInstance)
                {
                        HUDWidgetInstance->AddToViewport();

                        if (CharacterStatusWidgetClass)
                        {
                                UCharacterStatusWidget* NewStatusWidget = nullptr;

                                if (APlayerController* PC = Cast<APlayerController>(GetController()))
                                {
                                        NewStatusWidget = CreateWidget<UCharacterStatusWidget>(PC, CharacterStatusWidgetClass);
                                }
                                else
                                {
                                        NewStatusWidget = CreateWidget<UCharacterStatusWidget>(GetWorld(), CharacterStatusWidgetClass);
                                }

                                if (NewStatusWidget)
                                {
                                        CharacterStatusWidgetInstance = NewStatusWidget;
                                        HUDWidgetInstance->AddCharacterStatusWidget(NewStatusWidget);

                                        if (CharacterStatus)
                                        {
                                                NewStatusWidget->SetStatusComponent(CharacterStatus);
                                        }

                                        if (SkillSystem)
                                        {
                                                NewStatusWidget->SetSkillSystemComponent(SkillSystem);
                                        }

                                        SetCharacterStatusVisible(false);
                                }
                        }

                        if (InventoryWidgetClass)
                        {
                                UInventoryWidget* NewInventoryWidget = nullptr;

                                if (APlayerController* PC = Cast<APlayerController>(GetController()))
                                {
                                        NewInventoryWidget = CreateWidget<UInventoryWidget>(PC, InventoryWidgetClass);
                                }
                                else
                                {
                                        NewInventoryWidget = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
                                }

                                if (NewInventoryWidget)
                                {
                                        InventoryWidgetInstance = NewInventoryWidget;
                                        InventoryWidgetInstance->SetSkillSystemComponent(SkillSystem);
                                        InventoryWidgetInstance->SetInventoryComponent(Inventory);
                                        HUDWidgetInstance->AddLeftInventoryWidget(InventoryWidgetInstance);
                                        InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);

                                        if (!ContainerInventoryWidgetInstance)
                                        {
                                                UInventoryWidget* NewContainerWidget = nullptr;

                                                if (APlayerController* PC = Cast<APlayerController>(GetController()))
                                                {
                                                        NewContainerWidget = CreateWidget<UInventoryWidget>(PC, InventoryWidgetClass);
                                                }
                                                else
                                                {
                                                        NewContainerWidget = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
                                                }

                                                if (NewContainerWidget)
                                                {
                                                        ContainerInventoryWidgetInstance = NewContainerWidget;
                                                        ContainerInventoryWidgetInstance->SetAutoBindToOwningPawn(false);
                                                        ContainerInventoryWidgetInstance->SetSkillSystemComponent(SkillSystem);
                                                        ContainerInventoryWidgetInstance->SetInventoryComponent(nullptr);
                                                        ContainerInventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
                                                        HUDWidgetInstance->AddRightInventoryWidget(ContainerInventoryWidgetInstance);
                                                }
                                        }
                                }
                                UE_LOG(LogTemp, Display, TEXT("Inventory Widget Created??"));
                        }

                        if (GameMenuWidgetClass)
                        {
                                UGameMenuWidget* NewGameMenuWidget = nullptr;

                                if (APlayerController* PC = Cast<APlayerController>(GetController()))
                                {
                                        NewGameMenuWidget = CreateWidget<UGameMenuWidget>(PC, GameMenuWidgetClass);
                                }
                                else
                                {
                                        NewGameMenuWidget = CreateWidget<UGameMenuWidget>(GetWorld(), GameMenuWidgetClass);
                                }

                                if (NewGameMenuWidget)
                                {
                                        GameMenuWidgetInstance = NewGameMenuWidget;
                                        GameMenuWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
                                        HUDWidgetInstance->SetGameMenuWidget(GameMenuWidgetInstance);
                                }
                        }

                        HUDWidgetInstance->ConfigureSkillMenu(CharacterSkillMenuClass, SkillSystem);
                        HUDWidgetInstance->ConfigureInspectionCountdown(InspectionCountdownWidgetClass);
                }
        }

        if (Inventory)
        {
                Inventory->OnInventoryUpdated.AddDynamic(this, &AMO56Character::HandleInventoryUpdated);
                HandleInventoryUpdated();

                if (UGameInstance* GameInstance = GetGameInstance())
                {
                        if (UMO56SaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>())
                        {
                                if (GameMenuWidgetInstance)
                                {
                                        GameMenuWidgetInstance->SetSaveSubsystem(SaveSubsystem);
                                }
                        }
                }
        }
        else if (GameMenuWidgetInstance)
        {
                if (UGameInstance* GameInstance = GetGameInstance())
                {
                        if (UMO56SaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>())
                        {
                                GameMenuWidgetInstance->SetSaveSubsystem(SaveSubsystem);
                        }
                }
        }
}

void AMO56Character::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
        if (Inventory)
        {
                if (UGameInstance* GameInstance = GetGameInstance())
                {
                        if (UMO56SaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>())
                        {
                                SaveSubsystem->UnregisterInventoryComponent(Inventory);

                                if (SkillSystem)
                                {
                                        SaveSubsystem->UnregisterSkillComponent(SkillSystem);
                                }
                        }
                }
        }

        CloseActiveContainerInventory(false);

        if (Inventory)
        {
                Inventory->OnInventoryUpdated.RemoveDynamic(this, &AMO56Character::HandleInventoryUpdated);
        }

        if (CharacterStatusWidgetInstance)
        {
                CharacterStatusWidgetInstance->SetSkillSystemComponent(nullptr);
                CharacterStatusWidgetInstance->SetStatusComponent(nullptr);
                CharacterStatusWidgetInstance->RemoveFromParent();
                CharacterStatusWidgetInstance = nullptr;
        }

        if (InventoryWidgetInstance)
        {
                InventoryWidgetInstance->SetSkillSystemComponent(nullptr);
                InventoryWidgetInstance->SetInventoryComponent(nullptr);
                InventoryWidgetInstance = nullptr;
        }

        if (ContainerInventoryWidgetInstance)
        {
                ContainerInventoryWidgetInstance->SetSkillSystemComponent(nullptr);
                ContainerInventoryWidgetInstance->SetInventoryComponent(nullptr);
                ContainerInventoryWidgetInstance->RemoveFromParent();
                ContainerInventoryWidgetInstance = nullptr;
        }

        CloseWorldContextMenu();

        if (GameMenuWidgetInstance)
        {
                GameMenuWidgetInstance->RemoveFromParent();
                GameMenuWidgetInstance = nullptr;
        }

        if (HUDWidgetInstance)
        {
                HUDWidgetInstance->RemoveFromParent();
                HUDWidgetInstance = nullptr;
        }

        Super::EndPlay(EndPlayReason);
}

void AMO56Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);

        DOREPLIFETIME(AMO56Character, bIsPossessed);
        DOREPLIFETIME(AMO56Character, bEnableAI);
}

void AMO56Character::PossessedBy(AController* NewController)
{
        Super::PossessedBy(NewController);
        bIsPossessed = true;
        OnRep_IsPossessed();

        if (HasAuthority())
        {
                if (UGameInstance* GameInstance = GetGameInstance())
                {
                        if (UMO56SaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>())
                        {
                                if (AMO56PlayerController* MOController = Cast<AMO56PlayerController>(NewController))
                                {
                                        SaveSubsystem->RegisterPlayerCharacter(this, MOController);

                                        if (Inventory)
                                        {
                                                SaveSubsystem->RegisterInventoryComponent(Inventory, true, MOController->GetPlayerSaveId());
                                        }

                                        if (SkillSystem)
                                        {
                                                SaveSubsystem->RegisterSkillComponent(SkillSystem, MOController->GetPlayerSaveId());
                                        }
                                }
                        }
                }
        }
}

void AMO56Character::UnPossessed()
{
        Super::UnPossessed();
        bIsPossessed = false;
        OnRep_IsPossessed();
}

void AMO56Character::OnRep_IsPossessed()
{
        // Intentionally empty; blueprint subclasses can react via IsPossessedByPlayer().
}

void AMO56Character::SetEnableAI(bool bEnable)
{
        if (!HasAuthority())
        {
                UE_LOG(LogMO56, Warning, TEXT("SetEnableAI should be invoked on the authority."));
                return;
        }

        if (bEnableAI != bEnable)
        {
                bEnableAI = bEnable;
                ForceNetUpdate();
        }
}


void AMO56Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Existing bindings...
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMO56Character::Move);
		EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMO56Character::Look);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMO56Character::Look);

                // Interact binding (press E)
                if (InteractAction)
                {
                        EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AMO56Character::OnInteract);
                }

                if (InventoryAction)
                {
                        EIC->BindAction(InventoryAction, ETriggerEvent::Started, this, &AMO56Character::OnToggleInventory);
                }

                if (CharacterStatusAction)
                {
                        EIC->BindAction(CharacterStatusAction, ETriggerEvent::Started, this, &AMO56Character::OnToggleCharacterStatus);
                }

                if (GameMenuAction)
                {
                        EIC->BindAction(GameMenuAction, ETriggerEvent::Started, this, &AMO56Character::OnToggleGameMenu);
                }
                else if (MenuAction)
                {
                        EIC->BindAction(MenuAction, ETriggerEvent::Started, this, &AMO56Character::OnToggleGameMenu);
                }

                if (SkillAction)
                {
                        EIC->BindAction(SkillAction, ETriggerEvent::Started, this, &AMO56Character::OnToggleSkillMenu);
                }
        }
}

void AMO56Character::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AMO56Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AMO56Character::OnInteract(const FInputActionValue& /*Value*/)
{
        SetInventoryVisible(false);
        SetCharacterStatusVisible(false);
        SetGameMenuVisible(false);

        FVector ViewLoc; FRotator ViewRot;

        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
                PC->GetPlayerViewPoint(ViewLoc, ViewRot);   // pawn viewpoint (good)
	}
	else if (FollowCamera)
	{
		ViewLoc = FollowCamera->GetComponentLocation();   // backup
		ViewRot = FollowCamera->GetComponentRotation();
	}
	else
	{
		return;
	}

	const float TraceDist = 600.f; // give yourself more room
	const FVector End = ViewLoc + ViewRot.Vector() * TraceDist;

	// Ignore self, return simple first
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractTrace), /*bTraceComplex*/ false, this);
	Params.AddIgnoredActor(this);

	// What kinds of things can we hit?
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjParams.AddObjectTypesToQuery(ECC_Pawn); // if NPCs can be interactables

	FHitResult Hit;
	bool bHit = false;

	// --- Option A: line trace by object type (simple + clear) ---
	bHit = GetWorld()->LineTraceSingleByObjectType(Hit, ViewLoc, End, ObjParams, Params);

	// --- Option B: small sphere sweep (uncomment to try more forgiving detection) ---
	// const float Radius = 18.f;
	// const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	// bHit = GetWorld()->SweepSingleByObjectType(Hit, ViewLoc, End, FQuat::Identity, ObjParams, Shape, Params);
	// DrawDebugSphere(GetWorld(), End, Radius, 12, FColor::Cyan, false, 2.f, 0, 1.f);

        // DrawDebugLine(GetWorld(), ViewLoc, End, bHit ? FColor::Green : FColor::Red, false, 2.f, 0, 1.f);

        if (!bHit)
        {
                // UE_LOG(LogMO56, Log, TEXT("Interact trace MISS  Start:%s  End:%s"),
                //       *ViewLoc.ToString(), *End.ToString());
                return;
        }

        AActor* HitActor = Hit.GetActor();
         //UE_LOG(LogMO56, Log, TEXT("Interact trace HIT %s at %s")),
        //       *GetNameSafe(HitActor), *Hit.ImpactPoint.ToString());

        if (!HitActor)
        {
                return;
        }

        const bool bImplementsInteractable = HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass());

        if (HasAuthority())
        {
                if (bImplementsInteractable)
                {
                        UE_LOG(LogMO56, Display, TEXT("Character: Interact on server -> %s"), *GetNameSafe(HitActor));
                        IInteractable::Execute_Interact(HitActor, this);
                }
        }
        else
        {
                UE_LOG(LogMO56, Display, TEXT("Character: Interact on client -> RPC to server (%s)"), *GetNameSafe(HitActor));
                Server_Interact(HitActor);
        }
}

void AMO56Character::OnToggleInventory(const FInputActionValue& /*Value*/)
{

        UE_LOG(LogTemp, Display, TEXT("Trying to toggle the inventory widget"));
        if (!InventoryWidgetInstance)
        {
                return;
        }

        const bool bShouldShow = !InventoryWidgetInstance->IsVisible();
        if (bShouldShow)
        {
                SetGameMenuVisible(false);
        }
        SetInventoryVisible(bShouldShow);
}

void AMO56Character::OnToggleCharacterStatus(const FInputActionValue& /*Value*/)
{
        if (!CharacterStatusWidgetInstance)
        {
                UE_LOG(LogTemp, Display, TEXT("No valid status widget"));
                return;
        }

        UE_LOG(LogTemp, Display, TEXT("Toggling status widget"));
        const ESlateVisibility CurrentVisibility = CharacterStatusWidgetInstance->GetVisibility();
        const bool bCurrentlyVisible = CurrentVisibility != ESlateVisibility::Collapsed && CurrentVisibility != ESlateVisibility::Hidden;
        if (!bCurrentlyVisible)
        {
                SetGameMenuVisible(false);
        }
        SetCharacterStatusVisible(!bCurrentlyVisible);
}

void AMO56Character::OnToggleGameMenu(const FInputActionValue& /*Value*/)
{
        if (!GameMenuWidgetInstance)
        {
                return;
        }

        const ESlateVisibility CurrentVisibility = GameMenuWidgetInstance->GetVisibility();
        const bool bCurrentlyVisible = CurrentVisibility != ESlateVisibility::Collapsed && CurrentVisibility != ESlateVisibility::Hidden;

        SetGameMenuVisible(!bCurrentlyVisible);
}

void AMO56Character::OnToggleSkillMenu(const FInputActionValue& /*Value*/)
{
        if (!HUDWidgetInstance)
        {
                return;
        }

        const bool bCurrentlyVisible = HUDWidgetInstance->IsSkillMenuVisible();

        if (!bCurrentlyVisible)
        {
                SetInventoryVisible(false);
                SetCharacterStatusVisible(false);
                SetGameMenuVisible(false);
        }

        const bool bNowVisible = HUDWidgetInstance->ToggleSkillMenu();

        if (bNowVisible)
        {
                CloseWorldContextMenu();
        }

        UpdateInventoryInputState(IsAnyInventoryPanelVisible());
}

void AMO56Character::Server_Interact_Implementation(AActor* HitActor)
{
        if (!HitActor)
        {
                return;
        }

        const bool bImplementsInteractable = HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass());
        UE_LOG(LogMO56, Display, TEXT("Character::Server_Interact -> %s (Implements=%d)"),
                *GetNameSafe(HitActor), bImplementsInteractable);
        if (bImplementsInteractable)
        {
                IInteractable::Execute_Interact(HitActor, this);
        }
}

void AMO56Character::HandleInventoryUpdated()
{
        if (InventoryWidgetInstance)
        {
                IInventoryUpdateInterface::Execute_OnUpdateInventory(InventoryWidgetInstance, Inventory);
        }

        OnInventoryUpdated();
}

void AMO56Character::Tick(float DeltaSeconds)
{
        Super::Tick(DeltaSeconds);

        if (bContextFocusActive && !FocusedContextPawn.IsValid() && ActiveWorldContextMenu)
        {
                CloseWorldContextMenu();
        }

        if (CharacterStatus)
        {
                const FVector Velocity = GetVelocity();
                const float CurrentSpeed = Velocity.Size();

                float CurrentSlope = 0.f;

                if (const UCharacterMovementComponent* MovementComp = GetCharacterMovement())
                {
                        if (MovementComp->CurrentFloor.bBlockingHit)
                        {
                                const FVector Normal = MovementComp->CurrentFloor.HitResult.ImpactNormal;
                                const float NormalZ = FMath::Max(KINDA_SMALL_NUMBER, Normal.Z);
                                const float Tangent = FMath::Sqrt(FMath::Max(0.f, 1.f - NormalZ * NormalZ)) / NormalZ;
                                CurrentSlope = FMath::Clamp(Tangent, 0.f, 1.f);
                        }
                }

                float CarriedLoad = 0.f;

                if (Inventory)
                {
                        CarriedLoad = Inventory->GetTotalWeight();
                }

                CharacterStatus->SetActivityInputs(CurrentSpeed, CurrentSlope, CarriedLoad);
        }

        APlayerController* PC = Cast<APlayerController>(GetController());

        const bool bInventoryVisible = InventoryWidgetInstance && InventoryWidgetInstance->IsVisible();
        const bool bContainerVisible = ContainerInventoryWidgetInstance && ContainerInventoryWidgetInstance->IsVisible();
        const bool bSkillMenuVisible = HUDWidgetInstance && HUDWidgetInstance->IsSkillMenuVisible();
        const bool bGameMenuVisible = GameMenuWidgetInstance && GameMenuWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed && GameMenuWidgetInstance->GetVisibility() != ESlateVisibility::Hidden;
        const bool bWorldMenuVisible = ActiveWorldContextMenu && ActiveWorldContextMenu->IsInViewport();

        FVector2D CursorPosition = FVector2D::ZeroVector;
        bool bHasCursorPosition = false;
        if (FSlateApplication::IsInitialized())
        {
                CursorPosition = FSlateApplication::Get().GetCursorPos();
                bHasCursorPosition = true;
        }

        bool bCursorOverInventory = false;
        bool bCursorOverSkillMenu = false;
        bool bCursorOverGameMenu = false;
        bool bCursorOverWorldMenu = false;

        if (bHasCursorPosition)
        {
                if (bInventoryVisible && InventoryWidgetInstance)
                {
                        bCursorOverInventory |= InventoryWidgetInstance->GetCachedGeometry().IsUnderLocation(CursorPosition);
                }

                if (bContainerVisible && ContainerInventoryWidgetInstance)
                {
                        bCursorOverInventory |= ContainerInventoryWidgetInstance->GetCachedGeometry().IsUnderLocation(CursorPosition);
                }

                if (bSkillMenuVisible && HUDWidgetInstance)
                {
                        if (UCharacterSkillMenu* SkillMenuWidget = HUDWidgetInstance->GetSkillMenuInstance())
                        {
                                bCursorOverSkillMenu = SkillMenuWidget->GetCachedGeometry().IsUnderLocation(CursorPosition);
                        }
                }

                if (bGameMenuVisible && GameMenuWidgetInstance)
                {
                        bCursorOverGameMenu = GameMenuWidgetInstance->GetCachedGeometry().IsUnderLocation(CursorPosition);
                }

                if (bWorldMenuVisible && ActiveWorldContextMenu)
                {
                        bCursorOverWorldMenu = ActiveWorldContextMenu->GetCachedGeometry().IsUnderLocation(CursorPosition);
                }
        }

        const bool bCursorOverAnyUI = bCursorOverInventory || bCursorOverSkillMenu || bCursorOverGameMenu || bCursorOverWorldMenu;

        const bool bRightClick = PC && PC->WasInputKeyJustPressed(EKeys::RightMouseButton);
        const bool bLeftClick = PC && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton);

        if (bRightClick && !bCursorOverAnyUI)
        {
                if (!TryOpenWorldContextMenu())
                {
                        CloseWorldContextMenu();
                }
        }

        if (ActiveWorldContextMenu && bLeftClick && !bCursorOverWorldMenu)
        {
                CloseWorldContextMenu();
        }

        if (bSkillMenuVisible && (bLeftClick || bRightClick) && !bCursorOverSkillMenu)
        {
                SetSkillMenuVisible(false);
        }

        if ((bInventoryVisible || bContainerVisible) && (bLeftClick || bRightClick) && !bCursorOverInventory)
        {
                SetInventoryVisible(false);
        }
}

void AMO56Character::SetInventoryVisible(bool bVisible)
{
        if (bVisible)
        {
                SetGameMenuVisible(false);
                SetSkillMenuVisible(false);
        }

        CloseWorldContextMenu();

        if (!InventoryWidgetInstance)
        {
                return;
        }

        const bool bCurrentlyVisible = InventoryWidgetInstance->IsVisible();

        if (bCurrentlyVisible != bVisible)
        {
                InventoryWidgetInstance->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        }

        if (!bVisible)
        {
                CloseActiveContainerInventory(true);
        }
        else if (ContainerInventoryWidgetInstance && ActiveContainerInventory.IsValid())
        {
                ContainerInventoryWidgetInstance->SetVisibility(ESlateVisibility::Visible);
        }

        UpdateInventoryInputState(IsAnyInventoryPanelVisible());
}

bool AMO56Character::IsAnyInventoryPanelVisible() const
{
        const bool bInventoryVisible = InventoryWidgetInstance && InventoryWidgetInstance->IsVisible();
        const bool bContainerVisible = ContainerInventoryWidgetInstance && ContainerInventoryWidgetInstance->IsVisible();
        return bInventoryVisible || bContainerVisible;
}

void AMO56Character::SetCharacterStatusVisible(bool bVisible)
{
        if (!CharacterStatusWidgetInstance)
        {
                return;
        }

        if (bVisible)
        {
                SetSkillMenuVisible(false);
                CloseWorldContextMenu();
        }

        const ESlateVisibility DesiredVisibility = bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;

        if (CharacterStatusWidgetInstance->GetVisibility() != DesiredVisibility)
        {
                CharacterStatusWidgetInstance->SetVisibility(DesiredVisibility);
        }
}

void AMO56Character::SetGameMenuVisible(bool bVisible)
{
        if (!GameMenuWidgetInstance)
        {
                return;
        }

        CloseWorldContextMenu();

        const ESlateVisibility DesiredVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
        const ESlateVisibility CurrentVisibility = GameMenuWidgetInstance->GetVisibility();
        const bool bCurrentlyVisible = CurrentVisibility != ESlateVisibility::Collapsed && CurrentVisibility != ESlateVisibility::Hidden;

        if (bCurrentlyVisible == bVisible)
        {
                UpdateInventoryInputState(IsAnyInventoryPanelVisible());
                return;
        }

        if (bVisible)
        {
                GameMenuWidgetInstance->SetVisibility(DesiredVisibility);
                SetInventoryVisible(false);
                SetCharacterStatusVisible(false);
                SetSkillMenuVisible(false);
        }
        else
        {
                GameMenuWidgetInstance->SetVisibility(DesiredVisibility);
        }

        UpdateInventoryInputState(IsAnyInventoryPanelVisible());
}

void AMO56Character::SetSkillMenuVisible(bool bVisible)
{
        if (!HUDWidgetInstance)
        {
                return;
        }

        if (bVisible)
        {
                CloseWorldContextMenu();
        }

        HUDWidgetInstance->SetSkillMenuVisibility(bVisible);
        UpdateInventoryInputState(IsAnyInventoryPanelVisible());
}

void AMO56Character::CloseWorldContextMenu()
{
        if (ActiveWorldContextMenu)
        {
                ActiveWorldContextMenu->DismissMenu();
        }
        else if (bContextFocusActive && FocusedContextPawn.IsValid())
        {
                if (AMO56PlayerController* PC = Cast<AMO56PlayerController>(GetController()))
                {
                        PC->NotifyPawnContextFocus(FocusedContextPawn.Get(), false);
                }

                FocusedContextPawn = nullptr;
                bContextFocusActive = false;
                UpdateInventoryInputState(IsAnyInventoryPanelVisible());
        }
}

void AMO56Character::HandleWorldContextMenuDismissed()
{
        if (ActiveWorldContextMenu)
        {
                ActiveWorldContextMenu->OnMenuDismissed.RemoveAll(this);
                ActiveWorldContextMenu = nullptr;
        }

        if (FocusedContextPawn.IsValid() && bContextFocusActive)
        {
                if (AMO56PlayerController* PC = Cast<AMO56PlayerController>(GetController()))
                {
                        PC->NotifyPawnContextFocus(FocusedContextPawn.Get(), false);
                }
        }

        FocusedContextPawn = nullptr;
        bContextFocusActive = false;
        UpdateInventoryInputState(IsAnyInventoryPanelVisible());
}

bool AMO56Character::TryOpenWorldContextMenu()
{
        if (!WorldContextMenuClass)
        {
                return false;
        }

        AMO56PlayerController* PC = Cast<AMO56PlayerController>(GetController());
        if (!PC)
        {
                return false;
        }

        FVector ViewLoc;
        FRotator ViewRot;
        PC->GetPlayerViewPoint(ViewLoc, ViewRot);

        const float TraceDist = 600.f;
        const FVector End = ViewLoc + ViewRot.Vector() * TraceDist;

        FCollisionQueryParams Params(SCENE_QUERY_STAT(WorldContextInspect), false, this);
        Params.AddIgnoredActor(this);

        FCollisionObjectQueryParams ObjParams;
        ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
        ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
        ObjParams.AddObjectTypesToQuery(ECC_Pawn);

        FHitResult Hit;
        if (!GetWorld()->LineTraceSingleByObjectType(Hit, ViewLoc, End, ObjParams, Params))
        {
                return false;
        }

        AActor* HitActor = Hit.GetActor();
        if (!HitActor)
        {
                return false;
        }

        UInspectableComponent* Inspectable = HitActor->FindComponentByClass<UInspectableComponent>();
        TArray<FSkillInspectionParams> InspectionParams;
        if (Inspectable && SkillSystem)
        {
                Inspectable->BuildInspectionParams(SkillSystem, InspectionParams);
        }

        TArray<UWorldActorContextMenuWidget::FContextAction> AdditionalActions;
        if (APawn* TargetPawn = Cast<APawn>(HitActor))
        {
                TWeakObjectPtr<AMO56PlayerController> WeakPC = PC;
                TWeakObjectPtr<APawn> WeakPawn = TargetPawn;

                AdditionalActions.Emplace(NSLOCTEXT("WorldContextMenu", "PossessPawn", "Possess"), [WeakPC, WeakPawn]()
                {
                        if (AMO56PlayerController* Controller = WeakPC.Get())
                        {
                                if (APawn* Pawn = WeakPawn.Get())
                                {
                                        Controller->RequestPossessPawn(Pawn);
                                }
                        }
                });

                AdditionalActions.Emplace(NSLOCTEXT("WorldContextMenu", "OpenPawnInventory", "Open Inventory"), [WeakPC, WeakPawn]()
                {
                        if (AMO56PlayerController* Controller = WeakPC.Get())
                        {
                                if (APawn* Pawn = WeakPawn.Get())
                                {
                                        Controller->RequestOpenPawnInventory(Pawn);
                                }
                        }
                });

                const bool bSwitchingPawn = FocusedContextPawn.Get() != TargetPawn;

                if (bContextFocusActive && bSwitchingPawn && FocusedContextPawn.IsValid())
                {
                        PC->NotifyPawnContextFocus(FocusedContextPawn.Get(), false);
                        bContextFocusActive = false;
                }

                FocusedContextPawn = TargetPawn;

                if (!bContextFocusActive || bSwitchingPawn)
                {
                        PC->NotifyPawnContextFocus(TargetPawn, true);
                        bContextFocusActive = true;
                }
        }
        else if (bContextFocusActive && FocusedContextPawn.IsValid())
        {
                PC->NotifyPawnContextFocus(FocusedContextPawn.Get(), false);
                bContextFocusActive = false;
                FocusedContextPawn = nullptr;
        }

        if (InspectionParams.Num() == 0 && AdditionalActions.Num() == 0)
        {
                return false;
        }

        if (ActiveWorldContextMenu)
        {
                ActiveWorldContextMenu->DismissMenu();
        }

        UWorldActorContextMenuWidget* MenuInstance = CreateWidget<UWorldActorContextMenuWidget>(PC, WorldContextMenuClass);
        if (!MenuInstance)
        {
                return false;
        }

        ActiveWorldContextMenu = MenuInstance;
        if (InspectionParams.Num() > 0)
        {
                ActiveWorldContextMenu->InitializeMenu(Inspectable, SkillSystem, InspectionParams, MoveTemp(AdditionalActions));
        }
        else
        {
                ActiveWorldContextMenu->InitializeWithActions(MoveTemp(AdditionalActions));
        }

        ActiveWorldContextMenu->OnMenuDismissed.AddUObject(this, &AMO56Character::HandleWorldContextMenuDismissed);
        ActiveWorldContextMenu->AddToViewport(1000);

        FVector2D ViewportPosition = FVector2D::ZeroVector;
        float MouseX = 0.f;
        float MouseY = 0.f;
        if (UWidgetLayoutLibrary::GetMousePositionScaledByDPI(PC, MouseX, MouseY))
        {
                ViewportPosition = FVector2D(MouseX, MouseY);
        }
        else
        {
                ViewportPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(PC);
                const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(PC);
                if (ViewportScale > KINDA_SMALL_NUMBER)
                {
                        ViewportPosition /= ViewportScale;
                }
        }

        ActiveWorldContextMenu->SetAlignmentInViewport(FVector2D::ZeroVector);
        ActiveWorldContextMenu->SetPositionInViewport(ViewportPosition, false);

        PC->SetShowMouseCursor(true);
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;

        UpdateInventoryInputState(IsAnyInventoryPanelVisible());

        return true;
}

void AMO56Character::UpdateInventoryInputState(bool bInventoryVisible)
{
        APlayerController* PC = Cast<APlayerController>(GetController());

        if (!PC)
        {
                return;
        }

        const bool bMenuVisible = GameMenuWidgetInstance &&
                GameMenuWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed &&
                GameMenuWidgetInstance->GetVisibility() != ESlateVisibility::Hidden;

        const bool bSkillMenuVisible = HUDWidgetInstance && HUDWidgetInstance->IsSkillMenuVisible();

        const bool bWorldMenuVisible = ActiveWorldContextMenu && ActiveWorldContextMenu->IsInViewport();

        const bool bAnyUIVisible = bInventoryVisible || bMenuVisible || bSkillMenuVisible || bWorldMenuVisible;

        if (bAnyUIVisible)
        {
                PC->SetShowMouseCursor(true);
                PC->bEnableClickEvents = true;
                PC->bEnableMouseOverEvents = true;

                FInputModeGameAndUI InputMode;
                InputMode.SetHideCursorDuringCapture(false);
                InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

                if (bMenuVisible && GameMenuWidgetInstance)
                {
                        InputMode.SetWidgetToFocus(GameMenuWidgetInstance->TakeWidget());
                }
                else if (InventoryWidgetInstance && bInventoryVisible)
                {
                        InputMode.SetWidgetToFocus(InventoryWidgetInstance->TakeWidget());
                }
                else if (ContainerInventoryWidgetInstance && ContainerInventoryWidgetInstance->IsVisible())
                {
                        InputMode.SetWidgetToFocus(ContainerInventoryWidgetInstance->TakeWidget());
                }
                else if (HUDWidgetInstance && HUDWidgetInstance->IsSkillMenuVisible())
                {
                        if (UCharacterSkillMenu* SkillMenuWidget = HUDWidgetInstance->GetSkillMenuInstance())
                        {
                                InputMode.SetWidgetToFocus(SkillMenuWidget->TakeWidget());
                        }
                }

                PC->SetInputMode(InputMode);
        }
        else
        {
                FInputModeGameOnly InputMode;
                PC->SetInputMode(InputMode);

                PC->SetShowMouseCursor(false);
                PC->bEnableClickEvents = false;
                PC->bEnableMouseOverEvents = false;
        }
}

void AMO56Character::OpenContainerInventory(UInventoryComponent* ContainerInventory, AActor* ContainerActor)
{
        if (!ContainerInventory || !HUDWidgetInstance)
        {
                return;
        }

        ActiveContainerInventory = ContainerInventory;
        ActiveContainerActor = ContainerActor;

        if (IsLocallyControlled())
        {
                if (!ContainerInventoryWidgetInstance && InventoryWidgetClass)
                {
                        UInventoryWidget* NewContainerWidget = nullptr;

                        if (APlayerController* PC = Cast<APlayerController>(GetController()))
                        {
                                NewContainerWidget = CreateWidget<UInventoryWidget>(PC, InventoryWidgetClass);
                        }
                        else
                        {
                                NewContainerWidget = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
                        }

                        if (NewContainerWidget)
                        {
                                ContainerInventoryWidgetInstance = NewContainerWidget;
                                ContainerInventoryWidgetInstance->SetAutoBindToOwningPawn(false);
                                ContainerInventoryWidgetInstance->SetInventoryComponent(nullptr);
                                ContainerInventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
                                HUDWidgetInstance->AddRightInventoryWidget(ContainerInventoryWidgetInstance);
                        }
                }

                if (ContainerInventoryWidgetInstance)
                {
                        ContainerInventoryWidgetInstance->SetAutoBindToOwningPawn(false);
                        ContainerInventoryWidgetInstance->SetInventoryComponent(ContainerInventory);
                        ContainerInventoryWidgetInstance->SetVisibility(ESlateVisibility::Visible);
                }

                SetInventoryVisible(true);
        }
}

void AMO56Character::CloseContainerInventoryForActor(AActor* ContainerActor, bool bClosePlayerInventory)
{
        if (ActiveContainerActor.Get() != ContainerActor)
        {
                return;
        }

        CloseActiveContainerInventory(false);

        if (bClosePlayerInventory)
        {
                SetInventoryVisible(false);
        }
        else
        {
                UpdateInventoryInputState(IsAnyInventoryPanelVisible());
        }
}

void AMO56Character::CloseAllPlayerMenus()
{
        SetInventoryVisible(false);
        SetSkillMenuVisible(false);
        SetCharacterStatusVisible(false);
        SetGameMenuVisible(false);
        CloseWorldContextMenu();
}

void AMO56Character::CloseActiveContainerInventory(bool bNotifyContainer)
{
        if (ContainerInventoryWidgetInstance)
        {
                ContainerInventoryWidgetInstance->SetInventoryComponent(nullptr);
                ContainerInventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
        }

        if (bNotifyContainer)
        {
                if (AActor* ContainerActor = ActiveContainerActor.Get())
                {
                        if (AInventoryContainer* InventoryContainer = Cast<AInventoryContainer>(ContainerActor))
                        {
                                InventoryContainer->NotifyInventoryClosed(this);

                                if (!InventoryContainer->HasAuthority())
                                {
                                        if (AMO56PlayerController* MOController = Cast<AMO56PlayerController>(GetController()))
                                        {
                                                MOController->RequestContainerInventoryOwnership(InventoryContainer);
                                                MOController->NotifyContainerInventoryClosed(InventoryContainer);
                                        }
                                }
                        }
                }
        }

        ActiveContainerInventory = nullptr;
        ActiveContainerActor.Reset();

        UpdateInventoryInputState(IsAnyInventoryPanelVisible());
}

void AMO56Character::DoMove(float Right, float Forward)
{
        if (SkillSystem && SkillSystem->HasActiveInspection())
        {
                if (!FMath::IsNearlyZero(Right) || !FMath::IsNearlyZero(Forward))
                {
                        SkillSystem->CancelCurrentInspection(TEXT("InputInterrupt"));
                }
        }

        if (GetController() != nullptr)
        {
                // find out which way is forward
                const FRotator Rotation = GetController()->GetControlRotation();
                const FRotator YawRotation(0, Rotation.Yaw, 0);

                // get forward vector
                const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

                // get right vector
                const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

                // add movement
                AddMovementInput(ForwardDirection, Forward);
                AddMovementInput(RightDirection, Right);
        }
}

void AMO56Character::DoLook(float Yaw, float Pitch)
{
        if (SkillSystem && SkillSystem->HasActiveInspection())
        {
                if (!FMath::IsNearlyZero(Yaw) || !FMath::IsNearlyZero(Pitch))
                {
                        SkillSystem->CancelCurrentInspection(TEXT("InputInterrupt"));
                }
        }

        if (GetController() != nullptr)
        {
                // add yaw and pitch input to controller
                AddControllerYawInput(Yaw);
                AddControllerPitchInput(Pitch);
        }
}

void AMO56Character::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AMO56Character::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}
