// Implementation: Present the player's skills and knowledge progress. Bind SkillList, KnowledgeList,
// and info panel widgets in the blueprint, then assign the skill system component at runtime so the
// menu can populate entries and show history/progression details when players inspect a skill.
#include "UI/CharacterSkillMenu.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Skills/SkillSystemComponent.h"
#include "Skills/SkillTypes.h"
#include "UI/SkillListEntryWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Slate/SlateEnums.h"

UCharacterSkillMenu::UCharacterSkillMenu(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{
        SkillEntryWidgetClass = USkillListEntryWidget::StaticClass();
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
        HideSkillInfo();
}

void UCharacterSkillMenu::NativeConstruct()
{
        Super::NativeConstruct();
        RefreshSkillData();
        RefreshInspectionStatus();
        HideSkillInfo();
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
        HideSkillInfo();
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

                HideSkillInfo();
                return;
        }

        TArray<FSkillKnowledgeEntry> KnowledgeEntries;
        SkillSystem->GetKnowledgeEntries(KnowledgeEntries);
        RebuildKnowledgeList(KnowledgeEntries);

        TArray<FSkillDomainProgress> SkillEntries;
        SkillSystem->GetSkillEntries(SkillEntries);
        RebuildSkillList(SkillEntries);

        if (SkillEntries.Num() == 0 && KnowledgeEntries.Num() == 0)
        {
                HideSkillInfo();
        }
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
                const TSubclassOf<USkillListEntryWidget> EntryClass = SkillEntryWidgetClass ? SkillEntryWidgetClass : USkillListEntryWidget::StaticClass();
                USkillListEntryWidget* EntryWidget = CreateWidget<USkillListEntryWidget>(this, EntryClass);
                if (!EntryWidget)
                {
                        continue;
                }

                EntryWidget->SetupFromKnowledge(Entry);
                EntryWidget->OnInfoRequested().AddUObject(this, &UCharacterSkillMenu::HandleSkillInfoRequested);
                KnowledgeList->AddChild(EntryWidget);
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
                const TSubclassOf<USkillListEntryWidget> EntryClass = SkillEntryWidgetClass ? SkillEntryWidgetClass : USkillListEntryWidget::StaticClass();
                USkillListEntryWidget* EntryWidget = CreateWidget<USkillListEntryWidget>(this, EntryClass);
                if (!EntryWidget)
                {
                        continue;
                }

                EntryWidget->SetupFromSkill(Entry);
                EntryWidget->OnInfoRequested().AddUObject(this, &UCharacterSkillMenu::HandleSkillInfoRequested);
                SkillList->AddChild(EntryWidget);
        }
}

void UCharacterSkillMenu::HandleSkillInfoRequested(const FText& Title, const FText& Rank, const FText& History, const FText& Tips, TSoftObjectPtr<UTexture2D> Icon)
{
        ShowSkillInfo(Title, Rank, History, Tips, Icon);
}

void UCharacterSkillMenu::ShowSkillInfo(const FText& Title, const FText& Rank, const FText& History, const FText& Tips, TSoftObjectPtr<UTexture2D> Icon)
{
        if (SkillInfoTitleText)
        {
                SkillInfoTitleText->SetText(Title);
        }

        if (SkillInfoRankText)
        {
                SkillInfoRankText->SetText(Rank);
        }

        if (SkillInfoHistoryText)
        {
                SkillInfoHistoryText->SetText(History);
        }

        if (SkillInfoTipsText)
        {
                SkillInfoTipsText->SetText(Tips);
        }

        if (SkillInfoIcon)
        {
                if (UTexture2D* LoadedIcon = Icon.IsNull() ? nullptr : Icon.LoadSynchronous())
                {
                        SkillInfoIcon->SetBrushFromTexture(LoadedIcon);
                        SkillInfoIcon->SetVisibility(ESlateVisibility::Visible);
                }
                else
                {
                        SkillInfoIcon->SetBrushFromTexture(nullptr);
                        SkillInfoIcon->SetVisibility(ESlateVisibility::Collapsed);
                }
        }

        if (SkillInfoPanel)
        {
                SkillInfoPanel->SetVisibility(ESlateVisibility::Visible);
        }
}

void UCharacterSkillMenu::HideSkillInfo()
{
        if (SkillInfoTitleText)
        {
                SkillInfoTitleText->SetText(FText::GetEmpty());
        }

        if (SkillInfoRankText)
        {
                SkillInfoRankText->SetText(FText::GetEmpty());
        }

        if (SkillInfoHistoryText)
        {
                SkillInfoHistoryText->SetText(FText::GetEmpty());
        }

        if (SkillInfoTipsText)
        {
                SkillInfoTipsText->SetText(FText::GetEmpty());
        }

        if (SkillInfoIcon)
        {
                SkillInfoIcon->SetBrushFromTexture(nullptr);
                SkillInfoIcon->SetVisibility(ESlateVisibility::Collapsed);
        }

        if (SkillInfoPanel)
        {
                SkillInfoPanel->SetVisibility(ESlateVisibility::Collapsed);
        }
}
