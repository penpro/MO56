#include "UI/CharacterSkillMenu.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Skills/SkillSystemComponent.h"
#include "Skills/SkillTypes.h"

UCharacterSkillMenu::UCharacterSkillMenu(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{
}

void UCharacterSkillMenu::SetSkillSystemComponent(USkillSystemComponent* InSkillSystem)
{
        if (SkillSystem.Get() == InSkillSystem)
        {
                return;
        }

        if (USkillSystemComponent* Current = SkillSystem.Get())
        {
                Current->OnSkillStateChanged.RemoveDynamic(this, &UCharacterSkillMenu::HandleSkillStateChanged);
                Current->OnInspectionStateChanged.RemoveDynamic(this, &UCharacterSkillMenu::HandleInspectionStateChanged);
        }

        SkillSystem = InSkillSystem;

        if (USkillSystemComponent* NewSystem = SkillSystem.Get())
        {
                NewSystem->OnSkillStateChanged.AddDynamic(this, &UCharacterSkillMenu::HandleSkillStateChanged);
                NewSystem->OnInspectionStateChanged.AddDynamic(this, &UCharacterSkillMenu::HandleInspectionStateChanged);
        }

        RefreshSkillData();
        RefreshInspectionStatus();
}

void UCharacterSkillMenu::NativeConstruct()
{
        Super::NativeConstruct();
        RefreshSkillData();
        RefreshInspectionStatus();
        StartInspectionRefreshTimer();
}

void UCharacterSkillMenu::NativeDestruct()
{
        StopInspectionRefreshTimer();

        if (USkillSystemComponent* Current = SkillSystem.Get())
        {
                Current->OnSkillStateChanged.RemoveDynamic(this, &UCharacterSkillMenu::HandleSkillStateChanged);
                Current->OnInspectionStateChanged.RemoveDynamic(this, &UCharacterSkillMenu::HandleInspectionStateChanged);
        }

        SkillSystem.Reset();
        Super::NativeDestruct();
}

void UCharacterSkillMenu::HandleSkillStateChanged()
{
        RefreshSkillData();
}

void UCharacterSkillMenu::HandleInspectionStateChanged()
{
        RefreshInspectionStatus();
}

void UCharacterSkillMenu::RefreshSkillData()
{
        if (!SkillSystem.IsValid())
        {
                if (KnowledgeList)
                {
                        KnowledgeList->ClearChildren();
                }

                if (SkillList)
                {
                        SkillList->ClearChildren();
                }

                return;
        }

        TArray<FSkillKnowledgeEntry> KnowledgeEntries;
        SkillSystem->GetKnowledgeEntries(KnowledgeEntries);
        RebuildKnowledgeList(KnowledgeEntries);

        TArray<FSkillDomainProgress> SkillEntries;
        SkillSystem->GetSkillEntries(SkillEntries);
        RebuildSkillList(SkillEntries);
}

void UCharacterSkillMenu::RefreshInspectionStatus()
{
        if (!InspectionStatusText)
        {
                return;
        }

        if (!SkillSystem.IsValid())
        {
                InspectionStatusText->SetText(FText::GetEmpty());
                return;
        }

        TArray<FSkillInspectionProgress> ProgressEntries;
        SkillSystem->GetInspectionProgress(ProgressEntries);

        if (ProgressEntries.Num() == 0)
        {
                InspectionStatusText->SetText(NSLOCTEXT("SkillMenu", "NoInspections", "No active inspections."));
                return;
        }

        const FSkillInspectionProgress& Active = ProgressEntries[0];
        const float Remaining = FMath::Max(0.f, Active.Remaining);
        const FText KnowledgeName = !Active.KnowledgeDisplayName.IsEmpty() ? Active.KnowledgeDisplayName : SkillDefinitions::GetKnowledgeDisplayName(Active.KnowledgeId);
        InspectionStatusText->SetText(FText::Format(NSLOCTEXT("SkillMenu", "InspectionTimer", "Inspecting {0} ({1}s remaining)"), KnowledgeName, FText::AsNumber(FMath::RoundToInt(Remaining))));
}

void UCharacterSkillMenu::StartInspectionRefreshTimer()
{
        if (UWorld* World = GetWorld())
        {
                World->GetTimerManager().SetTimer(InspectionRefreshHandle, this, &UCharacterSkillMenu::HandleInspectionRefreshTimer, 0.25f, true);
        }
}

void UCharacterSkillMenu::StopInspectionRefreshTimer()
{
        if (UWorld* World = GetWorld())
        {
                World->GetTimerManager().ClearTimer(InspectionRefreshHandle);
        }
}

void UCharacterSkillMenu::HandleInspectionRefreshTimer()
{
        RefreshInspectionStatus();
}

void UCharacterSkillMenu::RebuildKnowledgeList(const TArray<FSkillKnowledgeEntry>& KnowledgeEntries)
{
        if (!KnowledgeList)
        {
                return;
        }

        KnowledgeList->ClearChildren();

        for (const FSkillKnowledgeEntry& Entry : KnowledgeEntries)
        {
                UTextBlock* RowText = NewObject<UTextBlock>(this, UTextBlock::StaticClass());
                if (!RowText)
                {
                        continue;
                }

                const int32 RoundedValue = FMath::RoundToInt(Entry.Value);
                RowText->SetText(FText::Format(NSLOCTEXT("SkillMenu", "KnowledgeRow", "{0}: {1}"), Entry.DisplayName, FText::AsNumber(RoundedValue)));
                KnowledgeList->AddChild(RowText);
        }
}

void UCharacterSkillMenu::RebuildSkillList(const TArray<FSkillDomainProgress>& SkillEntries)
{
        if (!SkillList)
        {
                return;
        }

        SkillList->ClearChildren();

        for (const FSkillDomainProgress& Entry : SkillEntries)
        {
                UTextBlock* RowText = NewObject<UTextBlock>(this, UTextBlock::StaticClass());
                if (!RowText)
                {
                        continue;
                }

                const int32 RoundedValue = FMath::RoundToInt(Entry.Value);
                RowText->SetText(FText::Format(NSLOCTEXT("SkillMenu", "SkillRow", "{0}: {1}"), Entry.DisplayName, FText::AsNumber(RoundedValue)));
                SkillList->AddChild(RowText);
        }
}
