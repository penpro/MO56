#include "Menu/MO56SaveListItemWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UMO56SaveListItemWidget::NativeConstruct()
{
        Super::NativeConstruct();

        if (EntryButton)
        {
                EntryButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleClicked);
                EntryButton->OnClicked.AddDynamic(this, &ThisClass::HandleClicked);
        }
}

void UMO56SaveListItemWidget::InitFromData(const FString& Title, const FString& Meta, const FGuid& InId)
{
        SaveId = InId;

        if (TitleText)
        {
                TitleText->SetText(FText::FromString(Title));
        }

        if (MetaText)
        {
                MetaText->SetText(FText::FromString(Meta));
        }
}

void UMO56SaveListItemWidget::HandleClicked()
{
        OnSaveChosen.Broadcast(SaveId);
}
