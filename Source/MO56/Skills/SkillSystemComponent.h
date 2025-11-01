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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInspectionStarted, const FText&, Description, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInspectionProgress, float, Elapsed, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInspectionCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInspectionCancelled, FName, Reason);

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

        /** Populates the menu-friendly domain entries including level progress. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        void GetSkillDomainEntries(TArray<FSkillDomainEntry>& OutEntries) const;

        /** Populates an array with active inspection progress entries. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        void GetInspectionProgress(TArray<FSkillInspectionProgress>& OutProgress) const;

        /** Returns true if the specified knowledge reward has already been claimed for the source. */
        UFUNCTION(BlueprintPure, Category = "Skills")
        bool HasCompletedInspectionForSource(const UObject* SourceContext, const FName& KnowledgeId) const;

        /** Cancels the first active inspection and reports the provided reason. */
        UFUNCTION(BlueprintCallable, Category = "Skills")
        void CancelCurrentInspection(const FName& Reason);

        /** Returns true if any inspection timers are currently active. */
        UFUNCTION(BlueprintPure, Category = "Skills")
        bool HasActiveInspection() const;

        /** Broadcast whenever a skill or knowledge value changes. */
        UPROPERTY(BlueprintAssignable, Category = "Skills")
        FOnSkillStateChanged OnSkillStateChanged;

        /** Broadcast when active inspection timers are added or removed. */
        UPROPERTY(BlueprintAssignable, Category = "Skills")
        FOnInspectionStateChanged OnInspectionStateChanged;

        /** Broadcast when an inspection countdown begins. */
        UPROPERTY(BlueprintAssignable, Category = "Skills")
        FOnInspectionStarted OnInspectionStarted;

        /** Broadcast periodically while an inspection is running. */
        UPROPERTY(BlueprintAssignable, Category = "Skills")
        FOnInspectionProgress OnInspectionProgress;

        /** Broadcast when an inspection finishes successfully. */
        UPROPERTY(BlueprintAssignable, Category = "Skills")
        FOnInspectionCompleted OnInspectionCompleted;

        /** Broadcast when an inspection is cancelled. */
        UPROPERTY(BlueprintAssignable, Category = "Skills")
        FOnInspectionCancelled OnInspectionCancelled;

        /** Serializes the component state to a save data struct. */
        void WriteToSaveData(FSkillSystemSaveData& OutData) const;

        /** Restores the component state from save data. */
        void ReadFromSaveData(const FSkillSystemSaveData& InData);

protected:
        virtual void BeginPlay() override;
        virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction) override;

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

        void NotifySaveSubsystemOfUpdate() const;

        void UpdateInspectionTickState();
        bool CancelInspectionForSource(UObject* SourceContext, const FName& Reason);
        void CancelInspectionInternal(const FGuid& InspectionId, const FName& Reason);
        void NotifyInspectionStarted(const FActiveInspection& InspectionEntry);
        void NotifyInspectionCancelled(const FActiveInspection& InspectionEntry, const FName& Reason);
        FText ResolveInspectionDescription(const FSkillInspectionParams& Params) const;
};
