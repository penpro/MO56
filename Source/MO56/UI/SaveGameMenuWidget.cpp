#include "UI/SaveGameMenuWidget.h"

#include "Components/ScrollBox.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MO56PlayerController.h"
#include "Save/MO56SaveGame.h"
#include "Save/MO56SaveSubsystem.h"
#include "UI/SaveGameDataWidget.h"

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
        APlayerController* PlayerController = GetOwningPlayer();
        if (!PlayerController)
        {
                PlayerController = UGameplayStatics::GetPlayerController(this, 0);
        }

        if (AMO56PlayerController* MOController = Cast<AMO56PlayerController>(PlayerController))
        {
                bool bRequested = false;

                if (Summary.SaveId.IsValid())
                {
                        bRequested = MOController->RequestLoadGameById(Summary.SaveId);
                }
                else
                {
                        bRequested = MOController->RequestLoadGameBySlot(Summary.SlotName, Summary.UserIndex);
                }

                if (bRequested)
                {
                        OnSaveLoaded.Broadcast();
                }

                return;
        }

        UE_LOG(LogTemp, Error,
                TEXT("SaveGameMenuWidget: No valid MO56 PlayerController. Set PlayerControllerClass to BP_MO56PlayerController in the Loading Screen map."));
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
