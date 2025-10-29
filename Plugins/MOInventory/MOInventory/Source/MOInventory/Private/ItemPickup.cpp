#include "ItemPickup.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DataTable.h"
#include "GameFramework/Pawn.h"
#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "MOItems/Public/ItemData.h"
#include "MOItems/Public/ItemTableRow.h"

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

void AItemPickup::ApplyRow(const FItemTableRow* Row)
{
    if (!Row) return;

    // Resolve and cache the gameplay asset
    UItemData* LoadedItem = Row->ItemAsset.LoadSynchronous();
    Item = LoadedItem;

    // Visuals
    if (UStaticMesh* SM = Row->StaticMesh.LoadSynchronous())
    {
        Mesh->SetStaticMesh(SM);
    }
    // If you prefer skeletal meshes, you can add an optional SkeletalMeshComponent
    // when Row->SkeletalMesh is set.

    // Optionally clamp quantity by stack size
    if (Item)
    {
        const int32 MaxStack = (Row->MaxStackOverride > 0) ? Row->MaxStackOverride
                                                          : FMath::Max(1, Item->MaxStackSize);
        Quantity = FMath::Clamp(Quantity, 1, MaxStack);
    }
}

void AItemPickup::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (ItemTable && !ItemRowName.IsNone())
    {
        if (const FItemTableRow* Row =
            ItemTable->FindRow<FItemTableRow>(ItemRowName, TEXT("ItemPickup")))
        {
            ApplyRow(Row);
        }
    }
}

#if WITH_EDITOR
void AItemPickup::RefreshFromRow()
{
    if (ItemTable && !ItemRowName.IsNone())
    {
        if (const FItemTableRow* Row = ItemTable->FindRow<FItemTableRow>(ItemRowName, TEXT("RefreshFromRow")))
        {
            ApplyRow(Row);
        }
    }
}
#endif

TArray<FName> AItemPickup::GetItemRowNames() const
{
    if (!ItemTable) return {};
    return ItemTable->GetRowNames();
}

bool AItemPickup::DoPickup(AActor* Interactor)
{

	UE_LOG(LogTemp, Display, TEXT("DoPickup called on ItemPickup"));
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
    if (Added <= 0)
    {
        return false;
    }

    if (Added >= Quantity)
    {
        Destroy();
        return true;
    }
    else
    {
        Quantity -= Added; // partial stack taken
    }

    return false;
}

void AItemPickup::Server_Interact_Implementation(AActor* Interactor)
{
    DoPickup(Interactor);

}

void AItemPickup::Interact_Implementation(AActor* Interactor)
{

    UE_LOG(LogTemp, Display, TEXT("interact called on ItemPickup"));
    if (!Interactor || !Item || Quantity <= 0)
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

void AItemPickup::OnRep_ItemRowName()
{
    if (ItemTable && !ItemRowName.IsNone())
    {
        if (const FItemTableRow* Row = ItemTable->FindRow<FItemTableRow>(ItemRowName, TEXT("OnRep_ItemRowName")))
        {
            ApplyRow(Row);
        }
    }
}

void AItemPickup::OnRep_Quantity()
{
    Quantity = FMath::Max(0, Quantity);
}

void AItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AItemPickup, ItemRowName);
    DOREPLIFETIME(AItemPickup, Quantity);
}

FText AItemPickup::GetInteractText_Implementation() const
{
    if (Item)
    {
        const FString Name = Item->DisplayName.IsEmpty() ? Item->GetName() : Item->DisplayName.ToString();
        return FText::FromString(FString::Printf(TEXT("Pick up %s x%d"), *Name, Quantity));
    }
    if (!ItemRowName.IsNone())
    {
        return FText::FromString(FString::Printf(TEXT("Pick up %s x%d"), *ItemRowName.ToString(), Quantity));
    }
    return FText::FromString(TEXT("Pick up"));
}
