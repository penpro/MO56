#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemData; // from MOItems

USTRUCT(BlueprintType)
struct FItemStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UItemData> Item = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Quantity = 0;

    bool IsEmpty() const { return Item == nullptr || Quantity <= 0; }
    int32 MaxStack() const; // uses Item->MaxStackSize if present (else 1)
};

UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class MOINVENTORY_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UInventoryComponent();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 AddItem(UItemData* Item, int32 Count);

    // Pointer-based remove/count (safest)
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 RemoveItem(UItemData* Item, int32 Count);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 CountItem(UItemData* Item) const;

    // Optional: remove/count by asset name (fallback if you only have names)
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 RemoveItemByAssetName(FName AssetName, int32 Count);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 CountItemByAssetName(FName AssetName) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    const TArray<FItemStack>& GetSlots() const { return Slots; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
    int32 MaxSlots = 24;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TArray<FItemStack> Slots;

private:
    int32 AddToExistingStacks(UItemData* Item, int32 Count);
    int32 AddToEmptySlots(UItemData* Item, int32 Count);
};
