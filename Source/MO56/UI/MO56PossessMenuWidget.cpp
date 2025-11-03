#include "UI/MO56PossessMenuWidget.h"

#include "Components/ListView.h"
#include "MO56PlayerController.h"

void UMO56PossessMenuWidget::SetList(const TArray<FMOPossessablePawnInfo>& In)
{
        Current = In;
        EntryObjects.Reset();

        if (!PawnList)
        {
                return;
        }

        PawnList->ClearListItems();

        for (const FMOPossessablePawnInfo& Info : Current)
        {
                UMO56PossessablePawnListEntry* Entry = NewObject<UMO56PossessablePawnListEntry>(this);
                Entry->SetInfo(Info);
                EntryObjects.Add(Entry);
                PawnList->AddItem(Entry);
        }

        PawnList->RequestRefresh();
}

void UMO56PossessMenuWidget::RequestPossessSelected()
{
        if (!PawnList)
        {
                return;
        }

        UObject* Selected = PawnList->GetSelectedItem();
        if (!Selected)
        {
                return;
        }

        if (UMO56PossessablePawnListEntry* Entry = Cast<UMO56PossessablePawnListEntry>(Selected))
        {
                if (AMO56PlayerController* Controller = GetOwningPlayer<AMO56PlayerController>())
                {
                        if (Entry->Info.PawnId.IsValid())
                        {
                                Controller->ServerRequestPossessPawnById(Entry->Info.PawnId);
                        }
                }
        }
}
