#include "ItemPickup.h"
#include "Components/StaticMeshComponent.h"
#include "InventoryComponent.h"
#include "MOItems/Public/ItemData.h"
#include "Net/UnrealNetwork.h"

AItemPickup::AItemPickup()
{
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);

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
        StartDropPhysics();
    }
    else if (Dropped)
    {
        FinishDropPhysics();
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
    Mesh->SetSimulatePhysics(true);

    GetWorldTimerManager().ClearTimer(DropPhysicsTimerHandle);
    GetWorldTimerManager().SetTimer(DropPhysicsTimerHandle, this, &AItemPickup::FinishDropPhysics, 3.f, false);
}

void AItemPickup::FinishDropPhysics()
{
    if (!Mesh)
    {
        Dropped = false;
        return;
    }

    Mesh->SetSimulatePhysics(false);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    Dropped = false;
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
    // If you want the Item asset to replicate too (only needed if it can change at runtime):
    // DOREPLIFETIME_CONDITION(AItemPickup, Item, COND_InitialOnly);
}
