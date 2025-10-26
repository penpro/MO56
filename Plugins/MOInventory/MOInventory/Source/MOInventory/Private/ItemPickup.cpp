

#include "ItemPickup.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"  
#include "InventoryComponent.h"
#include "ItemData.h"

AItemPickup::AItemPickup()
{
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);

    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionObjectType(ECC_WorldDynamic);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
}

void AItemPickup::Interact_Implementation(AActor* Interactor)
{
    if (!Interactor || !Item || Quantity <= 0) return;

    if (UInventoryComponent* Inv = Interactor->FindComponentByClass<UInventoryComponent>())
    {
        const int32 Added = Inv->AddItem(Item, Quantity);
        if (Added >= Quantity)
        {
            Destroy();
        }
        else if (Added > 0)
        {
            Quantity -= Added; // partial pickup
        }
    }
}

FText AItemPickup::GetInteractText_Implementation() const
{
    if (ItemData)
    {
        return FText::FromString(
            FString::Printf(TEXT("Pick up %s x%d"), *ItemData->GetName(), Quantity));
    }
    return FText::FromString(TEXT("Pick up"));
}