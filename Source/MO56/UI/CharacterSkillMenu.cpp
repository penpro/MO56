#include "UI/CharacterSkillMenu.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Skills/SkillSystemComponent.h"
#include "Skills/SkillTypes.h"
#include "Blueprint/WidgetTree.h"

UCharacterSkillMenu::UCharacterSkillMenu(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{
        SetCanTick(true);
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
}

void UCharacterSkillMenu::NativeDestruct()
{
        if (USkillSystemComponent* Current = SkillSystem.Get())
        {
                Current->OnSkillStateChanged.RemoveDynamic(this, &UCharacterSkillMenu::HandleSkillStateChanged);
                Current->OnInspectionStateChanged.RemoveDynamic(this, &UCharacterSkillMenu::HandleInspectionStateChanged);
        }

        SkillSystem.Reset();
        Super::NativeDestruct();
}

void UCharacterSkillMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
        Super::NativeTick(MyGeometry, InDeltaTime);

        const double Now = FPlatformTime::Seconds();
        if (Now - LastInspectionRefresh > 0.25)
        {
                RefreshInspectionStatus();
                LastInspectionRefresh = Now;
        }
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

void UCharacterSkillMenu::RebuildKnowledgeList(const TArray<FSkillKnowledgeEntry>& KnowledgeEntries)
{
        if (!KnowledgeList)
        {
                return;
        }

        KnowledgeList->ClearChildren();

        UWidgetTree* LocalWidgetTree = GetWidgetTree();

        for (const FSkillKnowledgeEntry& Entry : KnowledgeEntries)
        {
                UTextBlock* RowText = LocalWidgetTree ? LocalWidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()) : nullptr;
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

        UWidgetTree* LocalWidgetTree = GetWidgetTree();

        for (const FSkillDomainProgress& Entry : SkillEntries)
        {
                UTextBlock* RowText = LocalWidgetTree ? LocalWidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()) : nullptr;
                if (!RowText)
                {
                        continue;
                }

                const int32 RoundedValue = FMath::RoundToInt(Entry.Value);
                RowText->SetText(FText::Format(NSLOCTEXT("SkillMenu", "SkillRow", "{0}: {1}"), Entry.DisplayName, FText::AsNumber(RoundedValue)));
                SkillList->AddChild(RowText);
        }
}
