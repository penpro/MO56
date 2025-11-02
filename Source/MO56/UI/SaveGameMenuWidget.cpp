#include "UI/SaveGameMenuWidget.h"

#include "Components/ScrollBox.h"
#include "Engine/GameInstance.h"
#include "Save/MO56SaveSubsystem.h"
#include "Save/MO56SaveGame.h"
#include "UI/SaveGameDataWidget.h"
#include "GameFramework/PlayerController.h"
#include "MO56PlayerController.h"

void USaveGameMenuWidget::NativeConstruct()
{
        Super::NativeConstruct();

        if (!SaveEntryWidgetClass)
        {
                SaveEntryWidgetClass = USaveGameDataWidget::StaticClass();
        }

        RefreshSaveEntries();
}

void USaveGameMenuWidget::SetSaveSubsystem(UMO56SaveSubsystem* Subsystem)
{
        CachedSubsystem = Subsystem;
        RefreshSaveEntries();
}

void USaveGameMenuWidget::RefreshSaveEntries()
{
        if (!SaveList)
        {
                return;
        }

        TArray<FSaveGameSummary> Summaries;

        if (UMO56SaveSubsystem* Subsystem = ResolveSubsystem())
        {
                const TArray<FSaveIndexEntry> Entries = Subsystem->ListSaves();
                for (const FSaveIndexEntry& Entry : Entries)
                {
                        FSaveGameSummary Summary;
                        Summary.SlotName = Entry.SlotName;
                        Summary.UserIndex = 0;
                        Summary.LastLevelName = Entry.LevelName.IsEmpty() ? NAME_None : FName(*Entry.LevelName);
                        Summary.LastSaveTimestamp = Entry.UpdatedUtc;
                        Summary.InitialSaveTimestamp = Entry.UpdatedUtc;
                        Summary.TotalPlayTimeSeconds = Entry.TotalPlaySeconds;
                        Summary.InventoryCount = 0;
                        Summary.SaveId = Entry.SaveId;

                        if (UMO56SaveGame* Header = Subsystem->PeekSaveHeader(Entry.SaveId))
                        {
                                Summary.InitialSaveTimestamp = Header->CreatedUtc;
                                Summary.LastSaveTimestamp = Header->UpdatedUtc;
                                Summary.TotalPlayTimeSeconds = Header->TotalPlayTimeSeconds;
                                Summary.LastLevelName = Header->LevelName.IsEmpty() ? NAME_None : FName(*Header->LevelName);
                                Summary.InventoryCount = Header->InventoryStates.Num();
                        }

                        Summaries.Add(Summary);
                }
        }

        RebuildEntries(Summaries);
}

void USaveGameMenuWidget::RebuildEntries(const TArray<FSaveGameSummary>& Summaries)
{
        if (!SaveList)
        {
                return;
        }

        SaveList->ClearChildren();
        EntryWidgets.Reset();

        if (!SaveEntryWidgetClass)
        {
                return;
        }

        for (const FSaveGameSummary& Summary : Summaries)
        {
                USaveGameDataWidget* EntryWidget = nullptr;

                if (APlayerController* PC = GetOwningPlayer())
                {
                        EntryWidget = CreateWidget<USaveGameDataWidget>(PC, SaveEntryWidgetClass);
                }
                else
                {
                        EntryWidget = CreateWidget<USaveGameDataWidget>(GetWorld(), SaveEntryWidgetClass);
                }

                if (!EntryWidget)
                {
                        continue;
                }

                EntryWidget->SetSummary(Summary);
                EntryWidget->OnLoadRequested.AddDynamic(this, &USaveGameMenuWidget::HandleEntryLoadRequested);

                SaveList->AddChild(EntryWidget);
                EntryWidgets.Add(EntryWidget);
        }
}

void USaveGameMenuWidget::HandleEntryLoadRequested(const FSaveGameSummary& Summary)
{
        bool bLoaded = false;

        if (AMO56PlayerController* PC = Cast<AMO56PlayerController>(GetOwningPlayer()))
        {
                if (Summary.SaveId.IsValid())
                {
                        bLoaded = PC->RequestLoadGameById(Summary.SaveId);
                }
                else
                {
                        bLoaded = PC->RequestLoadGameBySlot(Summary.SlotName, Summary.UserIndex);
                }
        }
        else if (UMO56SaveSubsystem* Subsystem = ResolveSubsystem())
        {
                if (Summary.SaveId.IsValid())
                {
                        Subsystem->LoadSave(Summary.SaveId);
                        bLoaded = true;
                }
        }

        if (bLoaded)
        {
                OnSaveLoaded.Broadcast();
        }
}

UMO56SaveSubsystem* USaveGameMenuWidget::ResolveSubsystem() const
{
        if (CachedSubsystem.IsValid())
        {
                return CachedSubsystem.Get();
        }

        if (UGameInstance* GameInstance = GetGameInstance())
        {
                return GameInstance->GetSubsystem<UMO56SaveSubsystem>();
        }

        return nullptr;
}
