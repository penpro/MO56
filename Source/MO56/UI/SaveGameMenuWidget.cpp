#include "UI/SaveGameMenuWidget.h"

#include "Components/ScrollBox.h"
#include "Engine/GameInstance.h"
#include "Save/MO56SaveSubsystem.h"
#include "UI/SaveGameDataWidget.h"
#include "GameFramework/PlayerController.h"

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
                Summaries = Subsystem->GetAvailableSaveSummaries();
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
        if (UMO56SaveSubsystem* Subsystem = ResolveSubsystem())
        {
                if (Subsystem->LoadGameBySlot(Summary.SlotName, Summary.UserIndex))
                {
                        OnSaveLoaded.Broadcast();
                }
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
