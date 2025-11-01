// Implementation: Manages server-authoritative skill and knowledge progression. Attach the
// component to player characters, hook the delegates for UI updates, and register with the save
// subsystem so per-player data persists across sessions.
#include "Skills/SkillSystemComponent.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "ItemData.h"
#include "Save/MO56SaveSubsystem.h"
#include "Skills/SkillTypes.h"
#include "TimerManager.h"
#include "UObject/Package.h"

namespace
{
        constexpr float MaxProgressValue = 100.f;
        constexpr float DefaultKnowledgeGain = 5.f;
        constexpr float DefaultInspectionDuration = 20.f;
        constexpr float DefaultInspectionSkillXP = 3.f;
}

USkillSystemComponent::USkillSystemComponent()
{
        PrimaryComponentTick.bCanEverTick = false;
        SetIsReplicatedByDefault(true);
        InitializeDefaults();
}

void USkillSystemComponent::BeginPlay()
{
        Super::BeginPlay();
        InitializeDefaults();
}

void USkillSystemComponent::InitializeDefaults()
{
        if (SkillValues.Num() == 0)
        {
                for (const FSkillDomainInfo& DomainInfo : SkillDefinitions::GetSkillDomains())
                {
                        SkillValues.Add(DomainInfo.Domain, 0.f);
                }
        }

        if (KnowledgeValues.Num() == 0)
        {
                for (const FKnowledgeInfo& KnowledgeInfo : SkillDefinitions::GetKnowledgeDefinitions())
                {
                        KnowledgeValues.Add(KnowledgeInfo.KnowledgeId, 0.f);
                }
        }
}

void USkillSystemComponent::GrantKnowledge(const FName& KnowledgeId, float Amount)
{
        if (KnowledgeId.IsNone() || Amount <= 0.f)
        {
                return;
        }

        float& Current = KnowledgeValues.FindOrAdd(KnowledgeId);
        const float NewValue = FMath::Clamp(Current + Amount, 0.f, MaxProgressValue);
        if (!FMath::IsNearlyEqual(NewValue, Current))
        {
                Current = NewValue;
                BroadcastSkillUpdate();
        }
}

void USkillSystemComponent::GrantSkillXP(ESkillDomain Domain, float Amount)
{
        if (Domain == ESkillDomain::None || Amount <= 0.f)
        {
                return;
        }

        float& Current = SkillValues.FindOrAdd(Domain);
        const float NewValue = FMath::Clamp(Current + Amount, 0.f, MaxProgressValue);
        if (!FMath::IsNearlyEqual(NewValue, Current))
        {
                Current = NewValue;
                BroadcastSkillUpdate();
        }
}

bool USkillSystemComponent::StartInspection(const FSkillInspectionParams& Params, UObject* SourceContext)
{
        if (!Params.IsValid())
        {
                return false;
        }

        return InternalStartInspection(Params, SourceContext);
}

bool USkillSystemComponent::StartItemInspection(const UItemData* ItemData, UObject* SourceContext)
{
        if (!ItemData)
        {
                return false;
        }

        FSkillInspectionParams Params = BuildParamsFromItem(*ItemData);
        if (!Params.IsValid())
        {
                return false;
        }

        return InternalStartInspection(Params, SourceContext);
}

bool USkillSystemComponent::StartInspectableInspection(UInspectableComponent* Inspectable, const FSkillInspectionParams& Params)
{
        if (!Inspectable)
        {
                return false;
        }

        return InternalStartInspection(Params, Inspectable);
}

void USkillSystemComponent::CancelInspection(UObject* SourceContext)
{
        if (!SourceContext)
        {
                return;
        }

        for (auto It = ActiveInspections.CreateIterator(); It; ++It)
        {
                FActiveInspection& Inspection = It.Value();
                if (Inspection.Source.Get() == SourceContext)
                {
                        if (UWorld* World = GetWorld())
                        {
                                World->GetTimerManager().ClearTimer(Inspection.TimerHandle);
                        }

                        It.RemoveCurrent();
                        BroadcastInspectionUpdate();
                        break;
                }
        }
}

bool USkillSystemComponent::IsInspecting(const UObject* SourceContext) const
{
        if (!SourceContext)
        {
                return false;
        }

        for (const auto& Pair : ActiveInspections)
        {
                if (Pair.Value.Source.Get() == SourceContext)
                {
                        return true;
                }
        }

        return false;
}

float USkillSystemComponent::GetKnowledgeValue(const FName& KnowledgeId) const
{
        if (const float* Found = KnowledgeValues.Find(KnowledgeId))
        {
                return *Found;
        }

        return 0.f;
}

float USkillSystemComponent::GetSkillValue(ESkillDomain Domain) const
{
        if (const float* Found = SkillValues.Find(Domain))
        {
                return *Found;
        }

        return 0.f;
}

float USkillSystemComponent::CalculateSuccessChance(float BaseChance, ESkillDomain Domain, const FName& KnowledgeId, float ContextModifiers) const
{
        const float SkillComponent = 0.6f * GetSkillValue(Domain);
        const float KnowledgeComponent = 0.2f * GetKnowledgeValue(KnowledgeId);
        const float RawChance = BaseChance + SkillComponent + KnowledgeComponent + ContextModifiers;
        return FMath::Clamp(RawChance, 5.f, 95.f);
}

int32 USkillSystemComponent::CalculateOutputQuality(ESkillDomain Domain, float ToolConditionModifier) const
{
        const float SkillValue = GetSkillValue(Domain);
        const float BaseQuality = 10.f * (0.5f + 0.5f * (SkillValue / MaxProgressValue));
        const float Adjusted = BaseQuality * FMath::Clamp(ToolConditionModifier, 0.25f, 1.5f);
        return FMath::Clamp(FMath::FloorToInt(Adjusted), 0, 10);
}

float USkillSystemComponent::CalculateTimeCost(float BaseTimeSeconds, ESkillDomain Domain, float EnvironmentModifiers) const
{
        const float SkillValue = GetSkillValue(Domain);
        const float SkillFactor = 1.f - (SkillValue / 150.f);
        const float Adjusted = BaseTimeSeconds * SkillFactor * (1.f + EnvironmentModifiers);
        return FMath::Max(0.1f, Adjusted);
}

void USkillSystemComponent::GetKnowledgeEntries(TArray<FSkillKnowledgeEntry>& OutEntries) const
{
        OutEntries.Reset();
        OutEntries.Reserve(KnowledgeValues.Num());

        for (const auto& Pair : KnowledgeValues)
        {
                FSkillKnowledgeEntry& Entry = OutEntries.AddDefaulted_GetRef();
                Entry.KnowledgeId = Pair.Key;
                Entry.Value = Pair.Value;

                if (const FKnowledgeInfo* Info = SkillDefinitions::FindKnowledgeInfo(Pair.Key))
                {
                        Entry.DisplayName = Info->DisplayName;
                        Entry.History = Info->History;
                        Entry.ProgressionTips = Info->ProgressionTips;
                        Entry.Icon = Info->Icon;
                }
                else
                {
                        Entry.DisplayName = FText::FromName(Pair.Key);
                        Entry.History = FText::GetEmpty();
                        Entry.ProgressionTips = FText::GetEmpty();
                        Entry.Icon = TSoftObjectPtr<UTexture2D>();
                }
        }

        OutEntries.Sort([](const FSkillKnowledgeEntry& A, const FSkillKnowledgeEntry& B)
        {
                return A.DisplayName.ToString() < B.DisplayName.ToString();
        });
}

void USkillSystemComponent::GetSkillEntries(TArray<FSkillDomainProgress>& OutEntries) const
{
        OutEntries.Reset();
        OutEntries.Reserve(SkillValues.Num());

        for (const auto& Pair : SkillValues)
        {
                FSkillDomainProgress& Entry = OutEntries.AddDefaulted_GetRef();
                Entry.Domain = Pair.Key;
                Entry.Value = Pair.Value;
                Entry.RankText = SkillDefinitions::BuildRankText(Pair.Value);

                if (const FSkillDomainInfo* DomainInfo = SkillDefinitions::FindDomainInfo(Pair.Key))
                {
                        Entry.DisplayName = DomainInfo->DisplayName;
                        Entry.History = DomainInfo->History;
                        Entry.ProgressionTips = DomainInfo->ProgressionTips;
                        Entry.Icon = DomainInfo->Icon;
                }
                else
                {
                        const FName Tag = SkillDefinitions::GetSkillDomainTag(Pair.Key);
                        Entry.DisplayName = FText::FromName(Tag);
                        Entry.History = FText::GetEmpty();
                        Entry.ProgressionTips = FText::GetEmpty();
                        Entry.Icon = TSoftObjectPtr<UTexture2D>();
                }
        }

        OutEntries.Sort([](const FSkillDomainProgress& A, const FSkillDomainProgress& B)
        {
                return A.DisplayName.ToString() < B.DisplayName.ToString();
        });
}

void USkillSystemComponent::GetInspectionProgress(TArray<FSkillInspectionProgress>& OutProgress) const
{
        OutProgress.Reset();
        OutProgress.Reserve(ActiveInspections.Num());

        const double Now = FPlatformTime::Seconds();

        for (const auto& Pair : ActiveInspections)
        {
                const FActiveInspection& Inspection = Pair.Value;
                FSkillInspectionProgress& Entry = OutProgress.AddDefaulted_GetRef();
                Entry.KnowledgeId = Inspection.Params.KnowledgeId;
                Entry.KnowledgeDisplayName = !Inspection.Params.Description.IsEmpty() ? Inspection.Params.Description : SkillDefinitions::GetKnowledgeDisplayName(Inspection.Params.KnowledgeId);
                Entry.Duration = Inspection.Params.Duration;

                if (UWorld* World = GetWorld())
                {
                        if (World->GetTimerManager().IsTimerActive(Inspection.TimerHandle))
                        {
                                Entry.Remaining = World->GetTimerManager().GetTimerRemaining(Inspection.TimerHandle);
                                Entry.Elapsed = Inspection.Params.Duration - Entry.Remaining;
                        }
                        else
                        {
                                Entry.Elapsed = Inspection.Params.Duration;
                                Entry.Remaining = 0.f;
                        }
                }
                else
                {
                        const float ElapsedSeconds = static_cast<float>(Now - Inspection.StartTime);
                        Entry.Elapsed = FMath::Clamp(ElapsedSeconds, 0.f, Inspection.Params.Duration);
                        Entry.Remaining = FMath::Max(0.f, Inspection.Params.Duration - Entry.Elapsed);
                }
        }
}

void USkillSystemComponent::WriteToSaveData(FSkillSystemSaveData& OutData) const
{
        OutData.SkillValues = SkillValues;
        OutData.KnowledgeValues = KnowledgeValues;
}

void USkillSystemComponent::ReadFromSaveData(const FSkillSystemSaveData& InData)
{
        SkillValues = InData.SkillValues;
        KnowledgeValues = InData.KnowledgeValues;

        for (const FSkillDomainInfo& DomainInfo : SkillDefinitions::GetSkillDomains())
        {
                float& Value = SkillValues.FindOrAdd(DomainInfo.Domain);
                Value = FMath::Clamp(Value, 0.f, MaxProgressValue);
        }

        for (const FKnowledgeInfo& KnowledgeInfo : SkillDefinitions::GetKnowledgeDefinitions())
        {
                float& Value = KnowledgeValues.FindOrAdd(KnowledgeInfo.KnowledgeId);
                Value = FMath::Clamp(Value, 0.f, MaxProgressValue);
        }

        BroadcastSkillUpdate();
}

bool USkillSystemComponent::HasCompletedInspectionForSource(const UObject* SourceContext, const FName& KnowledgeId) const
{
        if (!SourceContext || KnowledgeId.IsNone())
        {
                return false;
        }

        RemoveExpiredSources();

        const TWeakObjectPtr<const UObject> SourcePtr(SourceContext);
        if (const TSet<FName>* CompletedSet = ClaimedKnowledgeBySource.Find(SourcePtr))
        {
                return CompletedSet->Contains(KnowledgeId);
        }

        return false;
}

void USkillSystemComponent::HandleInspectionCompleted(FGuid InspectionId)
{
        if (FActiveInspection* Inspection = ActiveInspections.Find(InspectionId))
        {
                CompleteInspectionInternal(*Inspection);
                ActiveInspections.Remove(InspectionId);
                BroadcastInspectionUpdate();
        }
}

void USkillSystemComponent::BroadcastSkillUpdate()
{
        OnSkillStateChanged.Broadcast();
        NotifySaveSubsystemOfUpdate();
}

void USkillSystemComponent::BroadcastInspectionUpdate()
{
        OnInspectionStateChanged.Broadcast();
}

void USkillSystemComponent::RemoveExpiredSources() const
{
        for (auto It = ClaimedKnowledgeBySource.CreateIterator(); It; ++It)
        {
                if (!It.Key().IsValid())
                {
                        It.RemoveCurrent();
                }
        }
}

void USkillSystemComponent::CompleteInspectionInternal(FActiveInspection& InspectionEntry)
{
        if (!InspectionEntry.Params.KnowledgeId.IsNone() && InspectionEntry.Params.KnowledgeGain > 0.f)
        {
                GrantKnowledge(InspectionEntry.Params.KnowledgeId, InspectionEntry.Params.KnowledgeGain);
        }

        for (const auto& Pair : InspectionEntry.Params.SkillXpRewards)
        {
                GrantSkillXP(Pair.Key, Pair.Value);
        }

        if (InspectionEntry.Params.bOncePerSource)
        {
                const TWeakObjectPtr<const UObject>& Source = InspectionEntry.Source;
                if (Source.IsValid())
                {
                        TSet<FName>& CompletedSet = ClaimedKnowledgeBySource.FindOrAdd(Source);
                        CompletedSet.Add(InspectionEntry.Params.KnowledgeId);
                }
        }
}

bool USkillSystemComponent::InternalStartInspection(const FSkillInspectionParams& Params, UObject* SourceContext)
{
        if (!Params.IsValid())
        {
                return false;
        }

        if (Params.bOncePerSource && SourceContext && HasCompletedInspectionForSource(SourceContext, Params.KnowledgeId))
        {
                return false;
        }

        if (SourceContext)
        {
                CancelInspection(SourceContext);
        }

        if (UWorld* World = GetWorld())
        {
                const FGuid InspectionId = FGuid::NewGuid();
                FActiveInspection& Inspection = ActiveInspections.Add(InspectionId);
                Inspection.Id = InspectionId;
                Inspection.Params = Params;
                Inspection.Source = SourceContext;
                Inspection.StartTime = FPlatformTime::Seconds();

                FTimerDelegate CompletionDelegate;
                CompletionDelegate.BindUObject(this, &USkillSystemComponent::HandleInspectionCompleted, InspectionId);
                World->GetTimerManager().SetTimer(Inspection.TimerHandle, CompletionDelegate, Params.Duration > 0.f ? Params.Duration : DefaultInspectionDuration, false);

                BroadcastInspectionUpdate();
                return true;
        }

        // No world or timers available, grant immediately.
        FActiveInspection Immediate;
        Immediate.Id = FGuid::NewGuid();
        Immediate.Params = Params;
        Immediate.Source = SourceContext;
        CompleteInspectionInternal(Immediate);
        BroadcastInspectionUpdate();
        return true;
}

FSkillInspectionParams USkillSystemComponent::BuildParamsFromItem(const UItemData& ItemData) const
{
        FSkillInspectionParams Params;
        Params.KnowledgeId = ItemData.KnowledgeTag;
        Params.KnowledgeGain = ItemData.InspectKnowledgeReward > 0.f ? ItemData.InspectKnowledgeReward : DefaultKnowledgeGain;
        Params.Duration = ItemData.InspectDuration > 0.f ? ItemData.InspectDuration : DefaultInspectionDuration;
        Params.Description = ItemData.DisplayName;

        const ESkillDomain PrimaryDomain = SkillDefinitions::ResolveSkillDomainFromTag(ItemData.PrimarySkillTag);
        if (PrimaryDomain != ESkillDomain::None)
        {
                Params.SkillXpRewards.Add(PrimaryDomain, DefaultInspectionSkillXP);
        }

        for (const auto& Pair : ItemData.AdditionalSkillXp)
        {
                const ESkillDomain Domain = SkillDefinitions::ResolveSkillDomainFromTag(Pair.Key);
                if (Domain != ESkillDomain::None && Pair.Value > 0.f)
                {
                        Params.SkillXpRewards.FindOrAdd(Domain) += Pair.Value;
                }
        }

        return Params;
}

void USkillSystemComponent::NotifySaveSubsystemOfUpdate() const
{
        if (UWorld* World = GetWorld())
        {
                if (UGameInstance* GameInstance = World->GetGameInstance())
                {
                        if (UMO56SaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>())
                        {
                                SaveSubsystem->NotifySkillComponentUpdated(const_cast<USkillSystemComponent*>(this));
                        }
                }
        }
}
