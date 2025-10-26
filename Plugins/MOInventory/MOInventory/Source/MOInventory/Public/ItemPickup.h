#pragma once

// ItemPickup.h

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "ItemPickup.generated.h"

class UStaticMeshComponent;
class UItemData;

UCLASS()
class MOINVENTORY_API AItemPickup : public AActor, public IInteractable
{
    GENERATED_BODY()

public:
    AItemPickup();

protected:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;

    UPROPERTY(EditAnywhere, Category = "Pickup")
    TObjectPtr<UItemData> Item;

    /** Which item this pickup gives */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
    UItemData* ItemData = nullptr;

    UPROPERTY(EditAnywhere, Category = "Pickup", meta = (ClampMin = "1"))
    int32 Quantity = 1;

public:
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractText_Implementation() const override;
};
