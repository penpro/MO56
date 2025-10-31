// InventoryComponent.h
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemData;
class UInventoryComponent;
struct FPropertyChangedEvent;
struct FTimerHandle;

USTRUCT(BlueprintType)
struct MOINVENTORY_API FItemStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UItemData> Item = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Quantity = 0;

    bool IsEmpty() const { return Item == nullptr || Quantity <= 0; }
    int32 MaxStack() const; // defined in .cpp
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class MOINVENTORY_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    virtual void InitializeComponent() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
    int32 MaxSlots = 24;

    /** Maximum total weight the inventory can hold (in kilograms). Set to 0 for unlimited. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Capacity", meta = (ClampMin = "0", ForceUnits = "kg"))
    float MaxWeight = 0.f;

    /** Maximum total volume the inventory can hold (in cubic meters). Set to 0 for unlimited. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Capacity", meta = (ClampMin = "0", ForceUnits = "m^3"))
    float MaxVolume = 0.f;

    /** Broadcast whenever the inventory contents change. */
    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryUpdated OnInventoryUpdated;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 AddItem(UItemData* Item, int32 Count);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 RemoveItem(UItemData* Item, int32 Count);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 CountItem(UItemData* Item) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool SplitStackAtIndex(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool DestroyItemAtIndex(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool TransferItemBetweenSlots(int32 SourceSlotIndex, int32 TargetSlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool DropItemAtIndex(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool DropSingleItemAtIndex(int32 SlotIndex);

    /** Moves an item from this inventory to the target inventory. */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool TransferItemToInventory(UInventoryComponent* TargetInventory, int32 SourceSlotIndex, int32 TargetSlotIndex);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    const TArray<FItemStack>& GetSlots() const { return Slots; }

    /** Returns the total weight of all items currently stored (in kilograms). */
    UFUNCTION(BlueprintPure, Category = "Inventory|Capacity")
    float GetTotalWeight() const;

    /** Returns the total volume of all items currently stored (in cubic meters). */
    UFUNCTION(BlueprintPure, Category = "Inventory|Capacity")
    float GetTotalVolume() const;

private:
    UPROPERTY(VisibleAnywhere, Category = "Inventory")
    TArray<FItemStack> Slots;

    int32 AddToExistingStacks(UItemData* Item, int32 Count);
    int32 AddToEmptySlots(UItemData* Item, int32 Count);
    void EnsureSlotCapacity();

    bool DropSingleItemInternal(int32 SlotIndex);
    void HandleDropAllTimerTick(int32 SlotIndex);
    void ClearDropAllTimer(int32 SlotIndex);

    TMap<int32, FTimerHandle> ActiveDropAllTimers;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
