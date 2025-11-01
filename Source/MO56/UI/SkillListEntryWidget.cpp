// Implementation: Helper widget backing skill and knowledge list rows. Blueprint designers
// bind optional icon/text/button widgets so the C++ code can populate them and notify the
// parent menu when the info button is pressed.
#include "UI/SkillListEntryWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Skills/SkillTypes.h"
#include "Engine/Texture2D.h"

USkillListEntryWidget::USkillListEntryWidget(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{
}

void USkillListEntryWidget::NativeConstruct()
{
        Super::NativeConstruct();
        UE_LOG(LogTemp, Display, TEXT("Skills List Created"));

        if (SkillInfoButton)
        {
                SkillInfoButton->OnClicked.AddDynamic(this, &USkillListEntryWidget::HandleInfoButtonClicked);
                SkillInfoButton->SetIsEnabled(true);
        }

        UpdateVisuals();
}

void USkillListEntryWidget::NativeDestruct()
{
        if (SkillInfoButton)
        {
                SkillInfoButton->OnClicked.RemoveDynamic(this, &USkillListEntryWidget::HandleInfoButtonClicked);
        }

        Super::NativeDestruct();
}

void USkillListEntryWidget::SetupFromSkill(const FSkillDomainProgress& Entry)
{
        ApplyDisplayData(Entry.DisplayName, Entry.RankText, Entry.History, Entry.ProgressionTips, Entry.Icon);
}

void USkillListEntryWidget::SetupFromKnowledge(const FSkillKnowledgeEntry& Entry)
{
        const int32 RoundedValue = FMath::RoundToInt(Entry.Value);
        const FText RankText = FText::Format(NSLOCTEXT("SkillMenu", "KnowledgeRank", "{0} pts"), FText::AsNumber(RoundedValue));
        ApplyDisplayData(Entry.DisplayName, RankText, Entry.History, Entry.ProgressionTips, Entry.Icon);
}

void USkillListEntryWidget::ApplyDisplayData(const FText& InName, const FText& InRank, const FText& InHistory, const FText& InTips, TSoftObjectPtr<UTexture2D> InIcon)
{
        StoredName = InName;
        StoredRank = InRank;
        StoredHistory = InHistory;
        StoredTips = InTips;
        StoredIcon = InIcon;

        UpdateVisuals();
}

void USkillListEntryWidget::UpdateVisuals()
{
        if (SkillName)
        {
                SkillName->SetText(StoredName);
        }

        if (SkillRank)
        {
                SkillRank->SetText(StoredRank);
        }

        if (SkillIcon)
        {
                if (UTexture2D* LoadedIcon = StoredIcon.IsNull() ? nullptr : StoredIcon.LoadSynchronous())
                {
                        SkillIcon->SetBrushFromTexture(LoadedIcon);
                        SkillIcon->SetVisibility(ESlateVisibility::Visible);
                }
                else
                {
                        SkillIcon->SetBrushFromTexture(nullptr);
                        SkillIcon->SetVisibility(ESlateVisibility::Collapsed);
                }
        }

        if (SkillInfoButton)
        {
                const bool bHasDetails = !StoredHistory.IsEmpty() || !StoredTips.IsEmpty();
                SkillInfoButton->SetIsEnabled(bHasDetails);
        }
}

void USkillListEntryWidget::HandleInfoButtonClicked()
{
        InfoRequested.Broadcast(StoredName, StoredRank, StoredHistory, StoredTips, StoredIcon);
}
