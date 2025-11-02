#pragma once

#include "Components/ActorComponent.h"
#include "Skills/SkillTypes.h"
#include "InspectableComponent.generated.h"

class USkillSystemComponent;

USTRUCT(BlueprintType)
struct FInspectionDiscovery
{
        GENERATED_BODY()

        /** Knowledge track to advance. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspect")
        FName KnowledgeTag = NAME_None;

        /** Amount of knowledge granted when inspection completes. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspect", meta = (ClampMin = "0"))
        float KnowledgeGain = 10.f;

        /** Optional skill tag that will receive XP. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspect")
        FName SkillTag = NAME_None;

        /** Skill XP awarded when inspection completes. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspect", meta = (ClampMin = "0"))
        float SkillXP = 5.f;

        /** Duration of the inspection timer in seconds. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspect", meta = (ClampMin = "0"))
        float Duration = 20.f;

        /** When true the inspection may only complete once per actor instance. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspect")
        bool bOncePerActor = true;

        /** Optional text description displayed in UI. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspect")
        FText Description;
};

/**
 * Component that exposes inspection discoveries for world actors.
 *
 * Editor Implementation Guide:
 * 1. Attach UInspectableComponent to Blueprint actors that players can study (plants, artifacts, workstations).
 * 2. Populate the Discoveries array with entries pointing to knowledge/skill tags defined in your skill data tables.
 * 3. Use Description to drive tooltip text in the context menu or inspection countdown overlay.
 * 4. Call BuildInspectionParams from Blueprint when opening UWorldActorContextMenuWidget to seed interaction buttons.
 * 5. For unique one-off discoveries, enable bOncePerActor so repeated inspections do not grant duplicate rewards.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MO56_API UInspectableComponent : public UActorComponent
{
        GENERATED_BODY()

public:
        /** Discoveries granted when the actor is inspected. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspect")
        TArray<FInspectionDiscovery> Discoveries;

        /** Builds inspection parameters for available discoveries, filtering against the supplied skill system. */
        void BuildInspectionParams(USkillSystemComponent* SkillSystem, TArray<FSkillInspectionParams>& OutParams) const;
};
