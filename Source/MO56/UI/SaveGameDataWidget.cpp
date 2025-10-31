#include "UI/SaveGameDataWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Internationalization/Text.h"
#include "Save/MO56SaveSubsystem.h"

void USaveGameDataWidget::NativeOnInitialized()
{
        Super::NativeOnInitialized();

        if (LoadButton)
        {
                LoadButton->OnClicked.AddDynamic(this, &USaveGameDataWidget::HandleLoadClicked);
        }
}

void USaveGameDataWidget::SetSummary(const FSaveGameSummary& Summary)
{
        CachedSummary = Summary;
        RefreshDisplay();
}

void USaveGameDataWidget::HandleLoadClicked()
{
        OnLoadRequested.Broadcast(CachedSummary);
}

void USaveGameDataWidget::RefreshDisplay()
{
        if (SlotNameText)
        {
                if (CachedSummary.SlotName.IsEmpty())
                {
                        SlotNameText->SetText(NSLOCTEXT("SaveGameDataWidget", "DefaultSlot", "Default Save"));
                }
                else
                {
                        SlotNameText->SetText(FText::FromString(CachedSummary.SlotName));
                }
        }

        if (TimePlayedText)
        {
                const FTimespan PlayTime = FTimespan::FromSeconds(CachedSummary.TotalPlayTimeSeconds);
                TimePlayedText->SetText(FText::AsTimespan(PlayTime));
        }

        if (LastSavedText)
        {
                LastSavedText->SetText(FormatDateTime(CachedSummary.LastSaveTimestamp));
        }

        if (InitialSavedText)
        {
                InitialSavedText->SetText(FormatDateTime(CachedSummary.InitialSaveTimestamp));
        }

        if (LastLevelText)
        {
                if (CachedSummary.LastLevelName.IsNone())
                {
                        LastLevelText->SetText(NSLOCTEXT("SaveGameDataWidget", "UnknownLevel", "Unknown"));
                }
                else
                {
                        LastLevelText->SetText(FText::FromName(CachedSummary.LastLevelName));
                }
        }

        if (InventoryCountText)
        {
                InventoryCountText->SetText(FText::AsNumber(CachedSummary.InventoryCount));
        }
}

FText USaveGameDataWidget::FormatDateTime(const FDateTime& DateTime) const
{
        if (DateTime.GetTicks() == 0)
        {
                return NSLOCTEXT("SaveGameDataWidget", "UnknownDate", "Unknown");
        }

        return FText::AsDateTime(DateTime);
}
