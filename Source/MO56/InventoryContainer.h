// Implementation: Place this actor in a level and assign a widget-driven inventory component
// in its blueprint. Characters that interact will open the container panel; add this class to
// actors that should expose shared loot between multiple players.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "InventoryContainer.generated.h"

class UInventoryComponent;
class USceneComponent;
class AMO56Character;

/**
 * Actor that exposes an inventory component and opens it when interacted with.
 */
UCLASS()
class MO56_API AInventoryContainer : public AActor, public IInteractable
{
        GENERATED_BODY()

public:
        AInventoryContainer();

        virtual void Interact_Implementation(AActor* Interactor) override;
        virtual FText GetInteractText_Implementation() const override;

        UFUNCTION(BlueprintPure, Category = "Inventory")
        UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

        /** Called when the interacting character closes the container UI. */
        UFUNCTION(BlueprintCallable, Category = "Inventory")
        void NotifyInventoryClosed(AMO56Character* Character);

protected:
        virtual void BeginPlay() override;
        virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

        UFUNCTION()
        void HandleInventoryUpdated();

        void HandleContainerEmptied();

protected:
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
        TObjectPtr<USceneComponent> Root;

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
        TObjectPtr<UInventoryComponent> InventoryComponent;

        /** If true, the container destroys itself when its inventory becomes empty. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
        bool bDestroyWhenEmpty = false;

        /** Customizable interact prompt shown to the player. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
        FText InteractPrompt;

        /** Characters currently viewing the container inventory. */
        TSet<TWeakObjectPtr<AMO56Character>> ActiveCharacters;
};
