#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Interactable.h"
#include "ItemPickup.generated.h"

class UStaticMeshComponent;
class UItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemPickupEvent, AItemPickup*, Pickup);

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

    /** True when the pickup originated from a player inventory drop. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup|Save")
    bool bWasSpawnedFromInventory = false;

    /** Persistent identifier used to reconcile save game data. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PersistentId, Category="Pickup|Save")
    FGuid PersistentId;

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Destroyed() override;

    void ApplyItemVisuals();

    void StartDropPhysics();
    void FinishDropPhysics();

    UFUNCTION()
    void OnRep_Quantity();

    UFUNCTION()
    void OnRep_PersistentId();

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

    UFUNCTION(BlueprintPure, Category="Pickup")
    UItemData* GetItem() const { return Item; }

    UFUNCTION(BlueprintPure, Category="Pickup")
    int32 GetQuantity() const { return Quantity; }

    UFUNCTION(BlueprintPure, Category="Pickup")
    bool WasSpawnedFromInventory() const { return bWasSpawnedFromInventory; }

    UFUNCTION(BlueprintCallable, Category="Pickup|Save")
    void SetWasSpawnedFromInventory(bool bInSpawnedFromInventory) { bWasSpawnedFromInventory = bInSpawnedFromInventory; }

    UFUNCTION(BlueprintPure, Category="Pickup|Save")
    FGuid GetPersistentId() const { return PersistentId; }

    UFUNCTION(BlueprintCallable, Category="Pickup|Save")
    void SetPersistentId(const FGuid& InPersistentId);

    UPROPERTY(BlueprintAssignable, Category="Pickup|Events")
    FItemPickupEvent OnDropSettled;

    UPROPERTY(BlueprintAssignable, Category="Pickup|Events")
    FItemPickupEvent OnPickupDestroyed;

private:
    FTimerHandle DropPhysicsTimerHandle;
};
