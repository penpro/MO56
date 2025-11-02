#include "Menu/MO56MainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Internationalization/Text.h"
#include "Save/MO56SaveSubsystem.h"

UMO56MainMenuWidget::UMO56MainMenuWidget(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{
        SetIsFocusable(true); // allow focusing the root
}

void UMO56MainMenuWidget::NativeConstruct()
{
        Super::NativeConstruct();

        NewGameButtonWidget = Cast<UButton>(GetWidgetFromName(TEXT("NewGameButton")));
        if (UButton* const NewGameButton = NewGameButtonWidget.Get())
        {
                NewGameButtonWidget->OnClicked.AddDynamic(this, &ThisClass::HandleNewGameClicked);
        }
        else
        {
                UE_LOG(LogTemp, Warning, TEXT("%s missing NewGameButton binding"), *GetName());
        }

        LoadGameButtonWidget = Cast<UButton>(GetWidgetFromName(TEXT("LoadGameButton")));
        if (UButton* const LoadGameButton = LoadGameButtonWidget.Get())
        {
                LoadGameButtonWidget->OnClicked.AddDynamic(this, &ThisClass::HandleLoadClicked);
        }
        else
        {
                UE_LOG(LogTemp, Warning, TEXT("%s missing LoadGameButton binding"), *GetName());
        }

        SaveListWidget = Cast<UScrollBox>(GetWidgetFromName(TEXT("SaveList")));

        RefreshSaveEntries();
}

void UMO56MainMenuWidget::RefreshSaveEntries()
{
        UScrollBox* const SaveList = SaveListWidget.Get();
        if (!SaveList)
        {
                UE_LOG(LogTemp, Warning, TEXT("%s missing SaveList binding"), *GetName());
                SaveButtonIds.Empty();
                return;
        }

        SaveListWidget->ClearChildren();
        SaveButtonIds.Empty();
        PendingSaveButton.Reset();

        if (UMO56SaveSubsystem* SaveSubsystem = ResolveSubsystem())
        {
                const TArray<FSaveIndexEntry> Entries = SaveSubsystem->ListSaves();

                for (const FSaveIndexEntry& Entry : Entries)
                {
                        if (!Entry.SaveId.IsValid())
                        {
                                continue;
                        }

                        UButton* EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
                        if (!EntryButton)
                        {
                                continue;
                        }

                        UTextBlock* EntryLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
                        if (EntryLabel)
                        {
                                EntryLabel->SetText(FormatEntryText(Entry));
                                EntryButton->AddChild(EntryLabel);
                        }

                        SaveListWidget->AddChild(EntryButton);

                        const FGuid SaveId = Entry.SaveId;
                        EntryButton->OnPressed.AddDynamic(this, &UMO56MainMenuWidget::HandleSaveEntryButtonPressed);
                        EntryButton->OnClicked.AddDynamic(this, &UMO56MainMenuWidget::HandleSaveEntryButtonClicked);

                        SaveButtonIds.Add(EntryButton, SaveId);
                }
        }
}

void UMO56MainMenuWidget::HandleSaveEntryButtonPressed()
{
        for (auto It = SaveButtonIds.CreateIterator(); It; ++It)
        {
                if (!It.Key().IsValid())
                {
                        It.RemoveCurrent();
                        continue;
                }

                if (It.Key()->IsPressed())
                {
                        PendingSaveButton = It.Key();
                        return;
                }
        }

        PendingSaveButton.Reset();
}

void UMO56MainMenuWidget::HandleSaveEntryButtonClicked()
{
        const TWeakObjectPtr<UButton> ClickedButton = PendingSaveButton;
        PendingSaveButton.Reset();

        if (!ClickedButton.IsValid())
        {
                return;
        }

        if (const FGuid* SaveId = SaveButtonIds.Find(ClickedButton))
        {
                HandleSaveEntryClicked(*SaveId);
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
        if (UMO56SaveSubsystem* SaveSubsystem = ResolveSubsystem())
        {
                SaveSubsystem->StartNewGame(TEXT("TestLevel"));
        }
}

void UMO56MainMenuWidget::HandleLoadClicked()
{
        RefreshSaveEntries();
}

void UMO56MainMenuWidget::NativeDestruct()
{
        if (UButton* const NewGameButton = NewGameButtonWidget.Get())
                NewGameButton->OnClicked.RemoveAll(this);
        if (UButton* const LoadGameButton = LoadGameButtonWidget.Get())
                LoadGameButton->OnClicked.RemoveAll(this);

        Super::NativeDestruct();
}

