#include "Menu/MO56MainMenuWidget.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Engine/GameInstance.h"
#include "Internationalization/Text.h"
#include "Kismet/GameplayStatics.h"
#include "Menu/MO56SaveListItemWidget.h"
#include "Save/MO56MenuSettingsSave.h"
#include "Save/MO56SaveSubsystem.h"

UMO56MainMenuWidget::UMO56MainMenuWidget(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{
        SetIsFocusable(true); // supported, non-deprecated
}

void UMO56MainMenuWidget::NativeConstruct()
{
        Super::NativeConstruct();

        if (NewGameButton)
        {
                NewGameButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleNewGameClicked);
                NewGameButton->OnClicked.AddDynamic(this, &ThisClass::HandleNewGameClicked);
        }
        else
        {
                UE_LOG(LogTemp, Warning, TEXT("%s missing NewGameButton binding"), *GetName());
        }

        if (LoadGameButton)
        {
                LoadGameButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleLoadClicked);
                LoadGameButton->OnClicked.AddDynamic(this, &ThisClass::HandleLoadClicked);
        }
        else
        {
                UE_LOG(LogTemp, Warning, TEXT("%s missing LoadGameButton binding"), *GetName());
        }

        if (!SaveList)
        {
                UE_LOG(LogTemp, Warning, TEXT("%s missing SaveList binding"), *GetName());
        }

        RefreshSaveEntries();
}

void UMO56MainMenuWidget::RefreshSaveEntries()
{
        if (!SaveList)
        {
                UE_LOG(LogTemp, Warning, TEXT("%s missing SaveList binding"), *GetName());
                return;
        }

        SaveList->ClearChildren();

        if (!SaveItemWidgetClass)
        {
                UE_LOG(LogTemp, Warning, TEXT("%s missing SaveItemWidgetClass"), *GetName());
                return;
        }

        if (UMO56SaveSubsystem* SaveSubsystem = ResolveSubsystem())
        {
                const TArray<FSaveIndexEntry> Entries = SaveSubsystem->ListSaves(true /*bRebuildFromDiskIfMissing*/);

                for (const FSaveIndexEntry& Entry : Entries)
                {
                        if (!Entry.SaveId.IsValid())
                        {
                                continue;
                        }

                        if (Entry.SlotName.Equals(UMO56MenuSettingsSave::StaticSlotName))
                        {
                                continue;
                        }

                        UMO56SaveListItemWidget* Row = CreateWidget<UMO56SaveListItemWidget>(GetOwningPlayer(), SaveItemWidgetClass);
                        if (!Row)
                        {
                                continue;
                        }

                        const FString TitleString = Entry.SlotName.IsEmpty() ? Entry.SaveId.ToString() : Entry.SlotName;
                        const FString MetaString = FormatEntryText(Entry).ToString();

                        Row->InitFromData(TitleString, MetaString, Entry.SaveId);
                        Row->OnSaveChosen.AddDynamic(this, &ThisClass::HandleSaveChosen);

                        SaveList->AddChild(Row);
                }
        }
}

void UMO56MainMenuWidget::HandleSaveEntryClicked(const FGuid& SaveId) const
{
        if (!SaveId.IsValid())
        {
                return;
        }

        if (UMO56SaveSubsystem* SaveSubsystem = ResolveSubsystem())
        {
                SaveSubsystem->LoadSave(SaveId);
        }
}

UMO56SaveSubsystem* UMO56MainMenuWidget::ResolveSubsystem() const
{
        if (CachedSubsystem.IsValid())
        {
                return CachedSubsystem.Get();
        }

        if (UGameInstance* GameInstance = GetGameInstance())
        {
                CachedSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>();
                return CachedSubsystem.Get();
        }

        return nullptr;
}

FText UMO56MainMenuWidget::FormatEntryText(const FSaveIndexEntry& Entry)
{
        const FText LevelText = Entry.LevelName.IsEmpty() ? NSLOCTEXT("MO56MainMenu", "UnknownLevel", "Unknown Level") : FText::FromString(Entry.LevelName);
        const FText UpdatedText = FormatDateTime(Entry.UpdatedUtc);
        const FText PlayTimeText = FText::AsTimespan(FTimespan::FromSeconds(Entry.TotalPlaySeconds));
        const FText IdText = FText::FromString(Entry.SaveId.ToString());

        return FText::Format(NSLOCTEXT("MO56MainMenu", "SaveEntryFormat", "{0}\nLevel: {1}\nUpdated: {2}\nPlaytime: {3}"), IdText, LevelText, UpdatedText, PlayTimeText);
}

FText UMO56MainMenuWidget::FormatDateTime(const FDateTime& DateTime)
{
        if (DateTime.GetTicks() == 0)
        {
                return NSLOCTEXT("MO56MainMenu", "UnknownDate", "Unknown");
        }

        return FText::AsDateTime(DateTime);
}

void UMO56MainMenuWidget::HandleNewGameClicked()
{
        FString Dest;
        if (StartingMap.IsValid() || StartingMap.ToSoftObjectPath().IsValid())
        {
                Dest = StartingMap.ToSoftObjectPath().GetAssetPathString();
        }
        else if (!StartingMapFallback.IsNone())
        {
                Dest = StartingMapFallback.ToString();
        }

        if (Dest.IsEmpty())
        {
                UE_LOG(LogTemp, Warning, TEXT("%s has no StartingMap configured."), *GetName());
                return;
        }

        if (UMO56SaveSubsystem* SaveSubsystem = ResolveSubsystem())
        {
                SaveSubsystem->StartNewGame(Dest);
                return;
        }

        if (APlayerController* PC = GetOwningPlayer())
        {
                PC->SetInputMode(FInputModeGameOnly());
                PC->bShowMouseCursor = false;
                PC->ClientTravel(Dest, TRAVEL_Absolute);
        }
        else
        {
                UGameplayStatics::OpenLevel(GetWorld(), FName(*Dest), /*bAbsolute*/true);
        }
}

void UMO56MainMenuWidget::HandleLoadClicked()
{
        RefreshSaveEntries();
}

void UMO56MainMenuWidget::HandleSaveChosen(FGuid SaveId)
{
        HandleSaveEntryClicked(SaveId);
}

void UMO56MainMenuWidget::NativeDestruct()
{
        if (NewGameButton)
        {
                NewGameButton->OnClicked.RemoveAll(this);
        }
        if (LoadGameButton)
        {
                LoadGameButton->OnClicked.RemoveAll(this);
        }

        Super::NativeDestruct();
}

