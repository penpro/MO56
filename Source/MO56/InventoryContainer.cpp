#include "InventoryContainer.h"

#include "InventoryComponent.h"
#include "MO56Character.h"
#include "Components/SceneComponent.h"
#include "Algo/AllOf.h"
#include "Internationalization/Text.h"

AInventoryContainer::AInventoryContainer()
{
        Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
        SetRootComponent(Root);

        InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
}

void AInventoryContainer::BeginPlay()
{
        Super::BeginPlay();

        if (InventoryComponent)
        {
                InventoryComponent->OnInventoryUpdated.AddDynamic(this, &AInventoryContainer::HandleInventoryUpdated);
                HandleInventoryUpdated();
        }
}

void AInventoryContainer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
        if (InventoryComponent)
        {
                InventoryComponent->OnInventoryUpdated.RemoveDynamic(this, &AInventoryContainer::HandleInventoryUpdated);
        }

        if (AMO56Character* Character = ActiveCharacter.Get())
        {
                Character->CloseContainerInventoryForActor(this);
                ActiveCharacter = nullptr;
        }

        Super::EndPlay(EndPlayReason);
}

void AInventoryContainer::HandleInventoryUpdated()
{
        if (!InventoryComponent)
        {
                return;
        }

        if (bDestroyWhenEmpty)
        {
                const TArray<FItemStack>& Slots = InventoryComponent->GetSlots();
                const bool bIsEmpty = Algo::AllOf(Slots, [](const FItemStack& Stack)
                {
                        return Stack.IsEmpty();
                });

                if (bIsEmpty)
                {
                        HandleContainerEmptied();
                }
        }
}

void AInventoryContainer::HandleContainerEmptied()
{
        if (AMO56Character* Character = ActiveCharacter.Get())
        {
                Character->CloseContainerInventoryForActor(this);
                ActiveCharacter = nullptr;
        }

        Destroy();
}

void AInventoryContainer::Interact_Implementation(AActor* Interactor)
{
        if (!Interactor)
        {
                return;
        }

        if (AMO56Character* Character = Cast<AMO56Character>(Interactor))
        {
                if (AMO56Character* PreviousCharacter = ActiveCharacter.Get())
                {
                        if (PreviousCharacter != Character)
                        {
                                PreviousCharacter->CloseContainerInventoryForActor(this);
                        }
                }

                ActiveCharacter = Character;
                Character->OpenContainerInventory(InventoryComponent, this);
        }
}

FText AInventoryContainer::GetInteractText_Implementation() const
{
        if (!InteractPrompt.IsEmpty())
        {
                return InteractPrompt;
        }

        return NSLOCTEXT("InventoryContainer", "InteractPrompt", "Open Container");
}

void AInventoryContainer::NotifyInventoryClosed(AMO56Character* Character)
{
        if (ActiveCharacter.Get() == Character)
        {
                ActiveCharacter = nullptr;
        }
}
