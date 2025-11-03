#include "UI/MO56PossessMenuWidget.h"

#include "Components/ListView.h"
#include "MO56.h"
#include "MO56PlayerController.h"

void UMO56PossessMenuWidget::NativeConstruct()
{
        Super::NativeConstruct();

        if (PawnList)
        {
                PawnList->OnItemClicked().AddUObject(this, &UMO56PossessMenuWidget::HandlePawnItemClicked);
        }
}

void UMO56PossessMenuWidget::NativeDestruct()
{
        if (PawnList)
        {
                PawnList->OnItemClicked().RemoveAll(this);
        }

        Super::NativeDestruct();
}

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
                HandlePawnItemClicked(Entry);
        }
}

void UMO56PossessMenuWidget::HandlePawnItemClicked(UObject* Item)
{
        if (!PawnList)
        {
                return;
        }

        UMO56PossessablePawnListEntry* Entry = Cast<UMO56PossessablePawnListEntry>(Item);
        if (!Entry)
        {
                return;
        }

        PawnList->SetSelectedItem(Entry);

        UE_LOG(LogMO56, Display, TEXT("PossessMenu: OnPawnItemClicked PawnId=%s"), *Entry->Info.PawnId.ToString());

        if (AMO56PlayerController* Controller = GetOwningPlayer<AMO56PlayerController>())
        {
                if (Entry->Info.PawnId.IsValid())
                {
                        Controller->ServerRequestPossessPawnById(Entry->Info.PawnId);
                }
        }
}
