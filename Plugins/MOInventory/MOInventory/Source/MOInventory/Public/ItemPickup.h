#pragma once

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<UItemData> Item = nullptr;

    UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Quantity, Category="Pickup", meta=(ClampMin="1"))
    int32 Quantity = 1;

    virtual void OnConstruction(const FTransform& Transform) override;

    void ApplyItemVisuals();

    UFUNCTION()
    void OnRep_Quantity();

public:
    UFUNCTION(BlueprintCallable, Category="Pickup")
    void SetItem(UItemData* NewItem);

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractText_Implementation() const override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    bool DoPickup(AActor* Interactor);

    UFUNCTION(Server, Reliable)
    void Server_Interact(AActor* Interactor);
};
