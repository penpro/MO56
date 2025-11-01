// Implementation: Attach this component to characters that need skill/knowledge tracking.
// Configure default values in C++, then use the blueprint-exposed getters to populate UI
// widgets such as skill menus and inspection progress panels.
#pragma once

#include "Components/ActorComponent.h"
#include "Skills/SkillTypes.h"
#include "SkillSystemComponent.generated.h"

class UItemData;
class UInspectableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInspectionStateChanged);

/**
 * Component responsible for tracking player knowledge and skill progression.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MO56_API USkillSystemComponent : public UActorComponent
{
        GENERATED_BODY()

public:
        USkillSystemComponent();

        /** Grants knowledge progress directly. Amount is clamped between 0 and 100. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        void GrantKnowledge(const FName& KnowledgeId, float Amount);

        /** Grants skill XP progress directly. Amount is clamped between 0 and 100. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        void GrantSkillXP(ESkillDomain Domain, float Amount);

        /** Starts an inspection timer from explicit parameters. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        bool StartInspection(const FSkillInspectionParams& Params, UObject* SourceContext);

        /** Attempts to start an inspection based on an item definition. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        bool StartItemInspection(const UItemData* ItemData, UObject* SourceContext);

        /** Attempts to start an inspection using an inspectable component. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        bool StartInspectableInspection(UInspectableComponent* Inspectable, const FSkillInspectionParams& Params);

        /** Cancels any inspection running for the specified source object. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        void CancelInspection(UObject* SourceContext);

        /** Returns true if an inspection is currently running for the specified source. */
        UFUNCTION(BlueprintPure, Category = "Skills")
        bool IsInspecting(const UObject* SourceContext) const;

        /** Returns the current value for a knowledge identifier (0..100). */
        UFUNCTION(BlueprintPure, Category = "Skills")
        float GetKnowledgeValue(const FName& KnowledgeId) const;

        /** Returns the current value for the specified skill domain (0..100). */
        UFUNCTION(BlueprintPure, Category = "Skills")
        float GetSkillValue(ESkillDomain Domain) const;

        /** Calculates the success chance of an action using the provided baseline. */
        UFUNCTION(BlueprintPure, Category = "Skills")
        float CalculateSuccessChance(float BaseChance, ESkillDomain Domain, const FName& KnowledgeId, float ContextModifiers) const;

        /** Calculates the quality tier of a crafted output. Returns an integer between 0 and 10. */
        UFUNCTION(BlueprintPure, Category = "Skills")
        int32 CalculateOutputQuality(ESkillDomain Domain, float ToolConditionModifier) const;

        /** Calculates the adjusted time cost based on skill, returning seconds. */
        UFUNCTION(BlueprintPure, Category = "Skills")
        float CalculateTimeCost(float BaseTimeSeconds, ESkillDomain Domain, float EnvironmentModifiers) const;

        /** Populates an array with the current knowledge state for UI consumption. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        void GetKnowledgeEntries(TArray<FSkillKnowledgeEntry>& OutEntries) const;

        /** Populates an array with current skill progress for UI consumption. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        void GetSkillEntries(TArray<FSkillDomainProgress>& OutEntries) const;

        /** Populates an array with active inspection progress entries. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        void GetInspectionProgress(TArray<FSkillInspectionProgress>& OutProgress) const;

        /** Returns true if the specified knowledge reward has already been claimed for the source. */
        UFUNCTION(BlueprintPure, Category = "Skills")
        bool HasCompletedInspectionForSource(const UObject* SourceContext, const FName& KnowledgeId) const;

        /** Broadcast whenever a skill or knowledge value changes. */
        UPROPERTY(BlueprintAssignable, Category = "Skills")
        FOnSkillStateChanged OnSkillStateChanged;

        /** Broadcast when active inspection timers are added or removed. */
        UPROPERTY(BlueprintAssignable, Category = "Skills")
        FOnInspectionStateChanged OnInspectionStateChanged;

protected:
        virtual void BeginPlay() override;

private:
        struct FActiveInspection
        {
                FGuid Id;
                FSkillInspectionParams Params;
                TWeakObjectPtr<UObject> Source;
                FTimerHandle TimerHandle;
                double StartTime = 0.0;
        };

        TMap<ESkillDomain, float> SkillValues;
        TMap<FName, float> KnowledgeValues;

        /** Active inspections keyed by unique identifier. */
        TMap<FGuid, FActiveInspection> ActiveInspections;

        /** Map tracking which knowledge rewards have been claimed per source object. */
        mutable TMap<TWeakObjectPtr<const UObject>, TSet<FName>> ClaimedKnowledgeBySource;

        void InitializeDefaults();

        void HandleInspectionCompleted(FGuid InspectionId);

        void BroadcastSkillUpdate();
        void BroadcastInspectionUpdate();

        void RemoveExpiredSources() const;

        void CompleteInspectionInternal(FActiveInspection& InspectionEntry);

        bool InternalStartInspection(const FSkillInspectionParams& Params, UObject* SourceContext);

        FSkillInspectionParams BuildParamsFromItem(const UItemData& ItemData) const;
};
