#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CraftingRecipeLibrary.generated.h"

class UCraftingRecipe;

UCLASS(BlueprintType)
class MO56_API UCraftingRecipeLibrary : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
    TArray<TObjectPtr<UCraftingRecipe>> Recipes;
};
