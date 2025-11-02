#include "Menu/MO56MainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Internationalization/Text.h"
#include "Save/MO56SaveSubsystem.h"

void UMO56MainMenuWidget::NativeConstruct()
{
        Super::NativeConstruct();

        if (!WidgetTree->RootWidget)
        {
                BuildMenuLayout();
        }

        if (NewGameButton)
        {
                NewGameButton->OnClicked.AddDynamic(this, &UMO56MainMenuWidget::HandleNewGameClicked);
        }

        if (LoadGameButton)
        {
                LoadGameButton->OnClicked.AddDynamic(this, &UMO56MainMenuWidget::HandleLoadClicked);
        }

        RefreshSaveEntries();
}

void UMO56MainMenuWidget::BuildMenuLayout()
{
        if (!WidgetTree)
        {
                return;
        }

        RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuRoot"));
        WidgetTree->RootWidget = RootBox;

        if (!RootBox)
        {
                return;
        }

        UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuTitle"));
        if (TitleText)
        {
                TitleText->SetText(NSLOCTEXT("MO56MainMenu", "MainMenuTitle", "Main Menu"));
                UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText);
                if (TitleSlot)
                {
                        TitleSlot->SetPadding(FMargin(0.f, 8.f));
                }
        }

        NewGameButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NewGameButton"));
        if (NewGameButton)
        {
                UTextBlock* NewGameLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NewGameLabel"));
                if (NewGameLabel)
                {
                        NewGameLabel->SetText(NSLOCTEXT("MO56MainMenu", "NewGame", "New Game"));
                        NewGameButton->AddChild(NewGameLabel);
                }

                UVerticalBoxSlot* ButtonSlot = RootBox->AddChildToVerticalBox(NewGameButton);
                if (ButtonSlot)
                {
                        ButtonSlot->SetPadding(FMargin(0.f, 8.f));
                }
        }

        LoadGameButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LoadGameButton"));
        if (LoadGameButton)
        {
                UTextBlock* LoadGameLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadGameLabel"));
                if (LoadGameLabel)
                {
                        LoadGameLabel->SetText(NSLOCTEXT("MO56MainMenu", "LoadGame", "Load Game"));
                        LoadGameButton->AddChild(LoadGameLabel);
                }

                UVerticalBoxSlot* ButtonSlot = RootBox->AddChildToVerticalBox(LoadGameButton);
                if (ButtonSlot)
                {
                        ButtonSlot->SetPadding(FMargin(0.f, 8.f));
                }
        }

        SaveList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SaveList"));
        if (SaveList)
        {
                UVerticalBoxSlot* ListSlot = RootBox->AddChildToVerticalBox(SaveList);
                if (ListSlot)
                {
                        ListSlot->SetPadding(FMargin(0.f, 12.f));
                        ListSlot->SetFillHeight(1.f);
                }
        }
}

void UMO56MainMenuWidget::RefreshSaveEntries()
{
        if (!SaveList)
        {
                return;
        }

        SaveList->ClearChildren();

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

                        SaveList->AddChild(EntryButton);

                        TWeakObjectPtr<UMO56MainMenuWidget> WeakThis(this);
                        const FGuid SaveId = Entry.SaveId;
                        EntryButton->OnClicked.AddLambda([WeakThis, SaveId]()
                        {
                                if (UMO56MainMenuWidget* StrongWidget = WeakThis.Get())
                                {
                                        StrongWidget->HandleSaveEntryClicked(SaveId);
                                }
                        });
                }
        }
}

void UMO56MainMenuWidget::HandleSaveEntryClicked(const FGuid& SaveId)
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

FText UMO56MainMenuWidget::FormatEntryText(const FSaveIndexEntry& Entry) const
{
        const FText LevelText = Entry.LevelName.IsEmpty() ? NSLOCTEXT("MO56MainMenu", "UnknownLevel", "Unknown Level") : FText::FromString(Entry.LevelName);
        const FText UpdatedText = FormatDateTime(Entry.UpdatedUtc);
        const FText PlayTimeText = FText::AsTimespan(FTimespan::FromSeconds(Entry.TotalPlaySeconds));
        const FText IdText = FText::FromString(Entry.SaveId.ToString());

        return FText::Format(NSLOCTEXT("MO56MainMenu", "SaveEntryFormat", "{0}\nLevel: {1}\nUpdated: {2}\nPlaytime: {3}"), IdText, LevelText, UpdatedText, PlayTimeText);
}

FText UMO56MainMenuWidget::FormatDateTime(const FDateTime& DateTime) const
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

