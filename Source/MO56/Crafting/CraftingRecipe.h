#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CraftingRecipe.generated.h"

class UTexture2D;
class AActor;

/** Data asset describing the inputs, outputs, and requirements for a craftable recipe. */
UCLASS(BlueprintType)
class MO56_API UCraftingRecipe : public UDataAsset
{
        GENERATED_BODY()

public:
        /** Localized display name used in menus. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
        FText DisplayName;

        /** Optional icon to represent the recipe. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
        TSoftObjectPtr<UTexture2D> Icon;

        /** Knowledge track required to view or execute the recipe. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Gating")
        FName RequiredKnowledge = NAME_None;

        /** Primary skill tag used when calculating success chance and XP. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Gating")
        FName PrimarySkillTag = NAME_None;

        /** Optional weighted secondary skill influences. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Gating")
        TMap<FName, int32> SecondarySkillWeights;

        /** Item identifiers and counts consumed when the craft succeeds or fails. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|IO")
        TMap<FName, int32> Inputs;

        /** Items granted when the craft succeeds. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|IO")
        TMap<FName, int32> Outputs;

        /** Items granted when the craft fails. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|IO")
        TMap<FName, int32> FailByproducts;

        /** Baseline duration of the craft in seconds. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Tuning")
        float BaseDuration = 5.f;

        /** Baseline difficulty used when evaluating success chance. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Tuning")
        float BaseDifficulty = 25.f;

        /** Primary skill XP granted when the craft succeeds. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Rewards")
        float SuccessXP = 5.f;

        /** Primary skill XP granted when the craft fails. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Rewards")
        float FailXP = 2.f;

        /** Knowledge gained whenever the craft is attempted. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Rewards")
        float KnowledgeOnTry = 0.f;

        /** Additional knowledge granted when the craft succeeds. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Rewards")
        float KnowledgeOnSuccess = 0.f;

        /** True when the recipe can be placed in the world as a buildable structure. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
        bool bIsBuildable = false;

        /** Actor spawned when the build site is completed. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building", meta = (EditCondition = "bIsBuildable"))
        TSubclassOf<AActor> BuildableActorClass;

        /** Materials required after the blueprint has been placed. */
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building", meta = (EditCondition = "bIsBuildable"))
        TMap<FName, int32> BuildMaterialRequirements;
};
