// ItemData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemData.generated.h"

// Basic rarity enum you can expand later.
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common     UMETA(DisplayName = "Common"),
	Uncommon   UMETA(DisplayName = "Uncommon"),
	Rare       UMETA(DisplayName = "Rare"),
	Epic       UMETA(DisplayName = "Epic"),
	Legendary  UMETA(DisplayName = "Legendary")
};

class UTexture2D;
class AActor;

/**
 * Simple item definition asset you can create via: Add ? Misc ? Data Asset ? ItemData
 * Uses PrimaryAssetId so it plays nicely with AssetManager (no .cpp needed).
 */
UCLASS(BlueprintType)
class MOITEMS_API UItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Display name shown to players/UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	// Optional longer text
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
	FText Description;

	// Small icon for UI. Raw pointer is fine here; forward-declared above.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// Stack size (1 for equipment / non-stackables)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	// In-game value / sell price, if applicable
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0"))
	int32 Value = 0;

	// Rarity tier
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EItemRarity Rarity = EItemRarity::Common;

	// Optional pickup actor to spawn when dropping this item
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftClassPtr<AActor> PickupActorClass;

public:
	// Make this a recognized primary asset without needing a .cpp definition.
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		// "Item" is your PrimaryAssetType; change if you want a different type.
		return FPrimaryAssetId(TEXT("Item"), GetFName());
	}
};
