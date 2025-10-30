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

#if WITH_EDITOR
void AItemPickup::PostEditChangeProperty(FPropertyChangedEvent& E)
{
    Super::PostEditChangeProperty(E);
    static const FName NAME_Item = GET_MEMBER_NAME_CHECKED(AItemPickup, Item);
    if (E.Property && E.Property->GetFName() == NAME_Item)
    {
        ApplyItemVisuals();              // live-update when you pick a new Item in Details
    }
}

void AItemPickup::PostActorCreated()
{
    Super::PostActorCreated();
    ApplyItemVisuals();                  // when you first place it in the level
}

void AItemPickup::RefreshVisuals()
{
    ApplyItemVisuals();                  // handy “Call in Editor” button
}
#endif

void AItemPickup::PostLoad()
{
    Super::PostLoad();
#if WITH_EDITOR
    ApplyItemVisuals();                  // when map loads in editor
#endif
}

void AItemPickup::PostInitProperties()
{
    Super::PostInitProperties();
#if WITH_EDITOR
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        ApplyItemVisuals();              // update the BP preview if Item is set on defaults
    }
#endif
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