#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "ItemPickup.generated.h"

class UStaticMeshComponent;
class UDataTable;
struct FItemTableRow;
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

    // Data table + row let you pick by name in the editor
    UPROPERTY(EditAnywhere, Category="Pickup|Data")
    UDataTable* ItemTable = nullptr;

    UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_ItemRowName, Category="Pickup|Data", meta=(GetOptions="GetItemRowNames"))
    FName ItemRowName;

    // Resolved gameplay item from the row (used by inventory)
    UPROPERTY(VisibleInstanceOnly, Category="Pickup|Resolved")
    TObjectPtr<UItemData> Item = nullptr;

    UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Quantity, Category="Pickup", meta=(ClampMin="1"))
    int32 Quantity = 1;

#if WITH_EDITOR
    UFUNCTION(CallInEditor)
    void RefreshFromRow();
#endif

    virtual void OnConstruction(const FTransform& Transform) override;

    void ApplyRow(const FItemTableRow* Row);

public:
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractText_Implementation() const override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Editor dropdown provider for RowName
    UFUNCTION()
    TArray<FName> GetItemRowNames() const;

private:
    /**
     * Tries to move the pickup into the interacting actor's inventory.
     * @return true if the full stack was transferred and the pickup should be destroyed on the server.
     */
    bool DoPickup(AActor* Interactor);

    UFUNCTION(Server, Reliable)
    void Server_Interact(AActor* Interactor);

    UFUNCTION()
    void OnRep_ItemRowName();

    UFUNCTION()
    void OnRep_Quantity();

};
