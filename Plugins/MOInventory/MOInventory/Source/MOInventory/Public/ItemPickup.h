#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
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

    // The ONLY source now: set this in your BP (e.g., BP_ApplePickup -> DA_Apple)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<UItemData> Item = nullptr;

    UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Quantity, Category="Pickup", meta=(ClampMin="1"))
    int32 Quantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup")
    bool Dropped = false;

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    void ApplyItemVisuals();

    void StartDropPhysics();
    void FinishDropPhysics();

    UFUNCTION()
    void OnRep_Quantity();

public:
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractText_Implementation() const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Optional: change the item at runtime
    UFUNCTION(BlueprintCallable, Category="Pickup")
    void SetItem(UItemData* NewItem);

    UFUNCTION(BlueprintCallable, Category="Pickup")
    void SetQuantity(int32 NewQuantity);

    UFUNCTION(BlueprintCallable, Category="Pickup")
    void SetDropped(bool bNewDropped);

private:
    FTimerHandle DropPhysicsTimerHandle;
};
