#include "ItemPickup.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "InventoryComponent.h"
#include "MOItems/Public/ItemData.h"
#include "Net/UnrealNetwork.h"

AItemPickup::AItemPickup()
{
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);

    Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionObjectType(ECC_WorldDynamic);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);

    bReplicates = true;
    SetReplicateMovement(true);
}

void AItemPickup::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyItemVisuals();
}

void AItemPickup::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        if (!PersistentId.IsValid())
        {
            PersistentId = FGuid::NewGuid();
        }
    }

    if (Dropped)
    {
        StartDropPhysics();
    }
}

void AItemPickup::ApplyItemVisuals()
{
    if (!Item) { Mesh->SetStaticMesh(nullptr); return; }

    if (UStaticMesh* SM = Item->WorldStaticMesh.LoadSynchronous())
    {
        Mesh->SetStaticMesh(SM);
        Mesh->SetWorldScale3D(Item->WorldScale3D);
        Mesh->SetRelativeRotation(Item->WorldRotationOffset);
    }
    // (If you add a SkeletalMeshComponent later, check Item->WorldSkeletalMesh here)

    Quantity = FMath::Clamp(Quantity, 1, FMath::Max(1, Item->MaxStackSize));
}

void AItemPickup::OnRep_Quantity()
{
    Quantity = FMath::Max(0, Quantity);
}

void AItemPickup::OnRep_PersistentId()
{
}

void AItemPickup::SetItem(UItemData* NewItem)
{
    Item = NewItem;
    ApplyItemVisuals();
}

void AItemPickup::SetQuantity(int32 NewQuantity)
{
    Quantity = FMath::Max(1, NewQuantity);
    OnRep_Quantity();
}

void AItemPickup::SetDropped(bool bNewDropped)
{
    if (bNewDropped)
    {
        bWasSpawnedFromInventory = true;
        StartDropPhysics();
    }
    else if (Dropped)
    {
        FinishDropPhysics();
    }
}

void AItemPickup::SetPersistentId(const FGuid& InPersistentId)
{
    PersistentId = InPersistentId;

    if (!HasAuthority())
    {
        OnRep_PersistentId();
    }
}

void AItemPickup::StartDropPhysics()
{
    if (!Mesh)
    {
        return;
    }

    Dropped = true;

    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
    Mesh->SetCollisionObjectType(ECC_PhysicsBody);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Mesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
    Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Mesh->SetSimulatePhysics(true);

    GetWorldTimerManager().ClearTimer(DropPhysicsTimerHandle);
    GetWorldTimerManager().SetTimer(DropPhysicsTimerHandle, this, &AItemPickup::FinishDropPhysics, 3.f, false);
}

void AItemPickup::FinishDropPhysics()
{
    if (!Mesh)
    {
        Dropped = false;
        OnDropSettled.Broadcast(this);
        return;
    }

    Mesh->SetSimulatePhysics(false);
    Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
    Mesh->SetCollisionObjectType(ECC_WorldDynamic);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    Dropped = false;
    OnDropSettled.Broadcast(this);
}

void AItemPickup::Destroyed()
{
    OnPickupDestroyed.Broadcast(this);
    Super::Destroyed();
}

void AItemPickup::Interact_Implementation(AActor* Interactor)
{
    if (!Interactor || Quantity <= 0) return;

    if (!HasAuthority())
    {
        // forward to the server (declare Server_Interact if you want RPC here)
        return;
    }

    UInventoryComponent* Inv = Interactor->FindComponentByClass<UInventoryComponent>();
    if (!Inv || !Item) return;

    const int32 Added = Inv->AddItem(Item, Quantity);
    if (Added <= 0) return;

    if (Added >= Quantity) { Destroy(); }
    else                   { Quantity -= Added; }
}

FText AItemPickup::GetInteractText_Implementation() const
{
    if (Item)
    {
        const FString Name = Item->DisplayName.IsEmpty() ? Item->GetName() : Item->DisplayName.ToString();
        return FText::FromString(FString::Printf(TEXT("Pick up %s x%d"), *Name, Quantity));
    }
    return FText::FromString(TEXT("Pick up"));
}

void AItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AItemPickup, Quantity);
    DOREPLIFETIME(AItemPickup, PersistentId);
    // If you want the Item asset to replicate too (only needed if it can change at runtime):
    // DOREPLIFETIME_CONDITION(AItemPickup, Item, COND_InitialOnly);
}
