#include "ItemPickup.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "InventoryComponent.h"
#include "MOItems/Public/ItemData.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"

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

void AItemPickup::ApplyItemVisuals()
{
    if (!Item)
    {
        Mesh->SetStaticMesh(nullptr);
        return;
    }

    if (UStaticMesh* SM = Item->WorldStaticMesh.LoadSynchronous())
    {
        Mesh->SetStaticMesh(SM);
        Mesh->SetWorldScale3D(Item->WorldScale3D);
        Mesh->SetRelativeRotation(Item->WorldRotationOffset);
    }

    Quantity = FMath::Clamp(Quantity, 1, FMath::Max(1, Item->MaxStackSize));
}

void AItemPickup::SetItem(UItemData* NewItem)
{
    Item = NewItem;
    ApplyItemVisuals();
}

bool AItemPickup::DoPickup(AActor* Interactor)
{
    UE_LOG(LogTemp, Display, TEXT("DoPickup: Item=%s Qty=%d Interactor=%s"),
        *GetNameSafe(Item), Quantity, *GetNameSafe(Interactor));
    if (!Interactor || !Item || Quantity <= 0)
    {
        return false;
    }

    UInventoryComponent* InventoryComponent = nullptr;

    if (APawn* PawnInteractor = Cast<APawn>(Interactor))
    {
        InventoryComponent = PawnInteractor->FindComponentByClass<UInventoryComponent>();
    }

    if (!InventoryComponent)
    {
        InventoryComponent = Interactor->FindComponentByClass<UInventoryComponent>();
    }

    if (!InventoryComponent)
    {
        return false;
    }

    const int32 Added = InventoryComponent->AddItem(Item, Quantity);
    UE_LOG(LogTemp, Display, TEXT("DoPickup: Added=%d"), Added);
    if (Added <= 0)
    {
        return false;
    }

    if (Added >= Quantity)
    {
        Destroy();
        return true;
    }

    Quantity -= Added; // partial stack taken
    return false;
}

void AItemPickup::Server_Interact_Implementation(AActor* Interactor)
{
    UE_LOG(LogTemp, Display, TEXT("Pickup::Server_Interact"));
    DoPickup(Interactor);
}

void AItemPickup::Interact_Implementation(AActor* Interactor)
{
    UE_LOG(LogTemp, Display, TEXT("Pickup::Interact (HasAuthority=%d)"), HasAuthority());
    if (!Interactor || Quantity <= 0)
    {
        return;
    }

    if (HasAuthority())
    {
        DoPickup(Interactor);
    }
    else
    {
        Server_Interact(Interactor);
    }
}

void AItemPickup::OnRep_Quantity()
{
    if (Item)
    {
        Quantity = FMath::Clamp(Quantity, 1, FMath::Max(1, Item->MaxStackSize));
    }
    else
    {
        Quantity = FMath::Max(0, Quantity);
    }
}

void AItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AItemPickup, Quantity);
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
