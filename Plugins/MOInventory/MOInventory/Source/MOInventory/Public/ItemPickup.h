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
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Runtime lifecycle (declare in header always)
    virtual void PostLoad() override;
    virtual void PostInitProperties() override;

#if WITH_EDITOR
    // Editor lifecycle (declare behind WITH_EDITOR)
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostActorCreated() override;

    UFUNCTION(CallInEditor, Category="Pickup")
    void RefreshVisuals();
#endif

    void ApplyItemVisuals();

    UFUNCTION()
    void OnRep_Quantity();

public:
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractText_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category="Pickup")
    void SetItem(UItemData* NewItem);
};
