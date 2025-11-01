#pragma once

#include "CoreMinimal.h"
#include "SkillTypes.generated.h"

class UTexture2D;

/**
 * Enumeration describing high-level survival skill domains.
 */
UENUM(BlueprintType)
enum class ESkillDomain : uint8
{
        None = 0,
        Naturalist,
        Foraging,
        Cordage,
        Knapping,
        Firecraft,
        Woodworking,
        Toolbinding,
        Watercrafting,
        Sheltercraft,
        Weaving,
        Tanning,
        TrappingFishing,
        Cooking,
        Clayworking
};

/**
 * Metadata describing a single skill domain used for UI presentation.
 */
USTRUCT(BlueprintType)
struct FSkillDomainInfo
{
        GENERATED_BODY()

        FSkillDomainInfo()
                : Domain(ESkillDomain::None)
        {
        }

        FSkillDomainInfo(ESkillDomain InDomain, const FName& InTag, const FText& InDisplayName,
                        const FText& InHistory, const FText& InProgressionTips, const TSoftObjectPtr<UTexture2D>& InIcon)
                : Domain(InDomain)
                , Tag(InTag)
                , DisplayName(InDisplayName)
                , History(InHistory)
                , ProgressionTips(InProgressionTips)
                , Icon(InIcon)
        {
        }

        /** Enumerated domain identifier. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        ESkillDomain Domain;

        /** Gameplay tag style identifier (Skill.DomainName). */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FName Tag;

        /** Localized domain display name. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText DisplayName;

        /** Lore or historical context for the domain. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText History;

        /** Guidance describing how to progress the domain. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText ProgressionTips;

        /** Optional icon used when rendering the domain. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        TSoftObjectPtr<UTexture2D> Icon;
};

/**
 * Metadata describing a knowledge track that the player can advance.
 */
USTRUCT(BlueprintType)
struct FKnowledgeInfo
{
        GENERATED_BODY()

        FKnowledgeInfo()
                : KnowledgeId(NAME_None)
        {
        }

        FKnowledgeInfo(const FName& InId, const FText& InDisplayName, const FName& InRelatedSkillTag,
                       const FText& InHistory, const FText& InProgressionTips, const TSoftObjectPtr<UTexture2D>& InIcon)
                : KnowledgeId(InId)
                , DisplayName(InDisplayName)
                , RelatedSkillTag(InRelatedSkillTag)
                , History(InHistory)
                , ProgressionTips(InProgressionTips)
                , Icon(InIcon)
        {
        }

        /** Unique identifier for the knowledge track. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FName KnowledgeId;

        /** Localized name used for the UI. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText DisplayName;

        /** Tag referencing the primary skill domain associated with this knowledge. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FName RelatedSkillTag;

        /** Lore or historical context used in info widgets. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText History;

        /** Tips explaining how to advance this knowledge. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText ProgressionTips;

        /** Optional icon used when rendering the knowledge entry. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        TSoftObjectPtr<UTexture2D> Icon;
};

/** Lightweight structure exposed to UI to display knowledge progress. */
USTRUCT(BlueprintType)
struct FSkillKnowledgeEntry
{
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FName KnowledgeId = NAME_None;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText DisplayName;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        float Value = 0.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText History;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText ProgressionTips;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        TSoftObjectPtr<UTexture2D> Icon;
};

/** Lightweight structure exposed to UI to display skill progress. */
USTRUCT(BlueprintType)
struct FSkillDomainProgress
{
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        ESkillDomain Domain = ESkillDomain::None;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText DisplayName;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        float Value = 0.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText RankText;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText History;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText ProgressionTips;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        TSoftObjectPtr<UTexture2D> Icon;
};

/** Aggregated skill domain entry exposed to menu widgets. */
USTRUCT(BlueprintType)
struct FSkillDomainEntry
{
        GENERATED_BODY()

        /** Enumerated domain identifier for reference. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        ESkillDomain Domain = ESkillDomain::None;

        /** Gameplay tag-style identifier (Skill.Domain). */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FName Tag = NAME_None;

        /** Localized display name of the domain. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText DisplayName;

        /** Current integer level derived from accumulated XP. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        int32 Level = 0;

        /** Progress made towards the next level within the current tier. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        float CurrentXP = 0.f;

        /** XP threshold required to reach the following level. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        float NextLevelXP = 0.f;

        /** Total accumulated XP across all levels. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        float TotalProgress = 0.f;
};

/** Data describing an active inspection timer. */
USTRUCT(BlueprintType)
struct FSkillInspectionProgress
{
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FName KnowledgeId = NAME_None;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        FText KnowledgeDisplayName;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        float Duration = 0.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        float Elapsed = 0.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
        float Remaining = 0.f;
};

/**
 * Parameter bundle describing a pending inspection request.
 */
USTRUCT(BlueprintType)
struct FSkillInspectionParams
{
        GENERATED_BODY()

        /** Knowledge track to advance when the inspection completes. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
        FName KnowledgeId = NAME_None;

        /** Amount of knowledge to grant on completion. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills", meta = (ClampMin = "0"))
        float KnowledgeGain = 5.f;

        /** Duration of the inspection timer (in seconds). */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills", meta = (ClampMin = "0"))
        float Duration = 20.f;

        /** Skill XP rewards granted on completion. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
        TMap<ESkillDomain, float> SkillXpRewards;

        /** When true the inspection can only complete once per source object. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
        bool bOncePerSource = false;

        /** Optional description shown in the UI for this inspection opportunity. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
        FText Description;

        bool IsValid() const
        {
                return !KnowledgeId.IsNone() && Duration >= 0.f;
        }
};

/** Serializable snapshot of a skill component. */
USTRUCT(BlueprintType)
struct FSkillSystemSaveData
{
        GENERATED_BODY()

        /** Stored skill domain progress (0..100). */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
        TMap<ESkillDomain, float> SkillValues;

        /** Stored knowledge values (0..100). */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
        TMap<FName, float> KnowledgeValues;
};

namespace SkillDefinitions
{
        /** Returns the static array of domain metadata. */
        const TArray<FSkillDomainInfo>& GetSkillDomains();

        /** Returns the static array of knowledge metadata used to seed the save state. */
        const TArray<FKnowledgeInfo>& GetKnowledgeDefinitions();

        /** Resolves the gameplay tag-like name for a skill domain. */
        FName GetSkillDomainTag(ESkillDomain Domain);

        /** Resolves a skill domain from the provided tag. */
        ESkillDomain ResolveSkillDomainFromTag(const FName& Tag);

        /** Returns the localized name for the provided knowledge identifier. */
        FText GetKnowledgeDisplayName(const FName& KnowledgeId);

        /** Resolves static metadata for a skill domain. */
        const FSkillDomainInfo* FindDomainInfo(ESkillDomain Domain);

        /** Resolves static metadata for a knowledge identifier. */
        const FKnowledgeInfo* FindKnowledgeInfo(const FName& KnowledgeId);

        /** Computes a rank descriptor based on the supplied value (0..100). */
        FText BuildRankText(float Value);
}
