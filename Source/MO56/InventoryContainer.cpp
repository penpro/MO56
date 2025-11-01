// Implementation: Add this actor to the level or blueprint, configure the inventory
// component slots in the editor, and hook up UI widgets to display its contents. Multiple
// characters can open the container simultaneously in multiplayer sessions.
#include "InventoryContainer.h"

#include "InventoryComponent.h"
#include "MO56Character.h"
#include "Components/SceneComponent.h"
#include "Algo/AllOf.h"
#include "Internationalization/Text.h"
#include "Save/MO56SaveSubsystem.h"
#include "Engine/GameInstance.h"

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

                if (UGameInstance* GameInstance = GetGameInstance())
                {
                        if (UMO56SaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>())
                        {
                                SaveSubsystem->RegisterInventoryComponent(InventoryComponent, false);
                        }
                }
        }
}

void AInventoryContainer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
        if (InventoryComponent)
        {
                if (UGameInstance* GameInstance = GetGameInstance())
                {
                        if (UMO56SaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>())
                        {
                                SaveSubsystem->UnregisterInventoryComponent(InventoryComponent);
                        }
                }
        }

        if (InventoryComponent)
        {
                InventoryComponent->OnInventoryUpdated.RemoveDynamic(this, &AInventoryContainer::HandleInventoryUpdated);
        }

        for (TWeakObjectPtr<AMO56Character>& CharacterPtr : ActiveCharacters)
        {
                if (AMO56Character* Character = CharacterPtr.Get())
                {
                        Character->CloseContainerInventoryForActor(this);
                }
        }
        ActiveCharacters.Empty();

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
        for (TWeakObjectPtr<AMO56Character>& CharacterPtr : ActiveCharacters)
        {
                if (AMO56Character* Character = CharacterPtr.Get())
                {
                        Character->CloseContainerInventoryForActor(this);
                }
        }
        ActiveCharacters.Empty();

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
                ActiveCharacters.Add(Character);
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
        ActiveCharacters.Remove(Character);
}
