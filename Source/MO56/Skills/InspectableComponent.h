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
