#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CraftingRecipeRows.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct FCraftingItemQty
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting", meta = (ClampMin = "0"))
    int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FCraftingRecipeRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    FName RecipeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    FName RequiredKnowledge = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    FName PrimarySkillTag = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    float BaseDuration = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    int32 BaseDifficulty = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    float KnowledgeOnTry = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    float KnowledgeOnSuccess = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    bool bIsBuildable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    TSoftClassPtr<AActor> BuildableActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    TArray<FCraftingItemQty> Inputs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    TArray<FCraftingItemQty> Outputs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    TArray<FCraftingItemQty> FailByproducts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    TArray<FCraftingItemQty> BuildMaterialRequirements;
};
