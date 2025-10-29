// Copyright Epic Games, Inc. All Rights Reserved.
// MO56Character.cpp

#include "MO56Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "MO56.h"

#include "Interactable.h"

#include "DrawDebugHelpers.h"

#include "InventoryComponent.h" // from MOInventory
#include "UI/HUDWidget.h"

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
                }
        }

        if (Inventory)
        {
                Inventory->OnInventoryUpdated.AddDynamic(this, &AMO56Character::HandleInventoryUpdated);
                HandleInventoryUpdated(Inventory);
        }
}

void AMO56Character::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
        if (Inventory)
        {
                Inventory->OnInventoryUpdated.RemoveDynamic(this, &AMO56Character::HandleInventoryUpdated);
        }

        if (HUDWidgetInstance)
        {
                HUDWidgetInstance->RemoveFromParent();
                HUDWidgetInstance = nullptr;
        }

        Super::EndPlay(EndPlayReason);
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
        // UE_LOG(LogMO56, Log, TEXT("Interact trace HIT %s at %s"),
        //       *GetNameSafe(HitActor), *Hit.ImpactPoint.ToString());

        if (HitActor && HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
        {
                IInteractable::Execute_Interact(HitActor, this);
                // UE_LOG(LogMO56, Log, TEXT("IInteractable::Interact executed on %s"), *GetNameSafe(HitActor));
        }
        else
        {
                // UE_LOG(LogMO56, Log, TEXT("Hit actor does not implement Interactable"));
        }
}

void AMO56Character::HandleInventoryUpdated(UInventoryComponent* UpdatedInventory)
{
        OnInventoryUpdated(UpdatedInventory);
}

void AMO56Character::DoMove(float Right, float Forward)
{
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
