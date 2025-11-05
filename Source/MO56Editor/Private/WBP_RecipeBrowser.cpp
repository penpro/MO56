#include "WBP_RecipeBrowser.h"

#if WITH_EDITOR

#include "Algo/Sort.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Crafting/CraftingRecipe.h"
#include "Crafting/CraftingRecipeLibrary.h"
#include "DesktopPlatformModule.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "IContentBrowserSingleton.h"
#include "IDesktopPlatform.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "RecipeEditorLibrary.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/TopLevelAssetPath.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

namespace RecipeBrowser
{
    static const FString DefaultRecipesRoot = TEXT("/Game/Crafting/Recipes");

    struct FCraftingRecipeListEntry
    {
        TWeakObjectPtr<UCraftingRecipe> Recipe;
        FString AssetPath;
        FString DisplayLabel;
        FString RecipeId;

        FText GetDisplayText() const
        {
            if (!DisplayLabel.IsEmpty())
            {
                return FText::FromString(DisplayLabel);
            }
            return FText::FromString(RecipeId);
        }
    };

    class SRecipeBrowserPanelImpl : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SRecipeBrowserPanelImpl) {}
        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs)
        {

            FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
            FDetailsViewArgs DetailsViewArgs;
            DetailsViewArgs.bHideSelectionTip = true;
            DetailsViewArgs.bUpdatesFromSelection = false;
            DetailsViewArgs.NotifyHook = nullptr;
            DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

            ChildSlot
            [
                SNew(SBorder)
                .Padding(4.f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.f, 0.f, 0.f, 4.f)
                    [
                        SAssignNew(SearchBox, SSearchBox)
                        .HintText(NSLOCTEXT("RecipeBrowser", "SearchHint", "Search recipes"))
                        .OnTextChanged(this, &SRecipeBrowserPanelImpl::OnSearchTextChanged)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.f, 0.f, 0.f, 4.f)
                    [
                        BuildToolbar()
                    ]
                    + SVerticalBox::Slot()
                    .FillHeight(1.f)
                    [
                        SNew(SSplitter)
                        + SSplitter::Slot()
                        .Value(0.35f)
                        [
                            SAssignNew(RecipeListView, SListView<TSharedPtr<FCraftingRecipeListEntry>>)
                            .ListItemsSource(&RecipeEntries)
                            .SelectionMode(ESelectionMode::Single)
                            .OnGenerateRow(this, &SRecipeBrowserPanelImpl::GenerateRecipeRow)
                            .OnSelectionChanged(this, &SRecipeBrowserPanelImpl::OnRecipeSelectionChanged)
                        ]
                        + SSplitter::Slot()
                        .Value(0.65f)
                        [
                            DetailsView.ToSharedRef()
                        ]
                    ]
                ]
            ];

            RefreshRecipeList();
        }

        void RefreshRecipeList()
        {
            TArray<UCraftingRecipe*> Assets;
            URecipeEditorLibrary::ListAllRecipeAssets(Assets);

            FString Filter = SearchText.ToString();
            Filter.TrimStartAndEndInline();
            Filter = Filter.ToLower();
            TWeakObjectPtr<UCraftingRecipe> CurrentlySelected = SelectedRecipe;

            RecipeEntries.Reset();

            for (UCraftingRecipe* Recipe : Assets)
            {
                if (!Recipe)
                {
                    continue;
                }

                const FString RecipeId = Recipe->GetName();
                FString DisplayLabel = Recipe->DisplayName.ToString();
                if (DisplayLabel.IsEmpty())
                {
                    DisplayLabel = RecipeId;
                }

                if (!Filter.IsEmpty())
                {
                    FString CandidateId = RecipeId;
                    CandidateId.ToLowerInline();

                    FString CandidateDisplay = DisplayLabel;
                    CandidateDisplay.ToLowerInline();

                    if (!CandidateId.Contains(Filter) && !CandidateDisplay.Contains(Filter))
                    {
                        continue;
                    }
                }

                TSharedPtr<FCraftingRecipeListEntry> Entry = MakeShared<FCraftingRecipeListEntry>();
                Entry->Recipe = Recipe;
                Entry->RecipeId = RecipeId;
                Entry->DisplayLabel = DisplayLabel;
                Entry->AssetPath = FPackageName::ObjectPathToPackageName(Recipe->GetPathName());
                RecipeEntries.Add(Entry);
            }

            RecipeEntries.Sort([](const TSharedPtr<FCraftingRecipeListEntry>& A, const TSharedPtr<FCraftingRecipeListEntry>& B)
            {
                return A->DisplayLabel < B->DisplayLabel;
            });

            if (RecipeListView.IsValid())
            {
                RecipeListView->RequestListRefresh();
            }

            if (CurrentlySelected.IsValid())
            {
                SelectRecipe(CurrentlySelected.Get());
            }
            else if (RecipeEntries.Num() > 0)
            {
                RecipeListView->SetSelection(RecipeEntries[0]);
            }
        }

        void SelectRecipe(UCraftingRecipe* Recipe)
        {
            if (!RecipeListView.IsValid())
            {
                return;
            }

            TSharedPtr<FCraftingRecipeListEntry> FoundEntry;
            for (const TSharedPtr<FCraftingRecipeListEntry>& Entry : RecipeEntries)
            {
                if (Entry.IsValid() && Entry->Recipe.Get() == Recipe)
                {
                    FoundEntry = Entry;
                    break;
                }
            }

            if (FoundEntry.IsValid())
            {
                RecipeListView->SetSelection(FoundEntry);
            }
            else
            {
                RecipeListView->ClearSelection();
                SelectedRecipe = nullptr;
                if (DetailsView.IsValid())
                {
                    DetailsView->SetObject(nullptr);
                }
            }
        }

    private:
        TSharedRef<SWidget> BuildToolbar()
        {
            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.f, 0.f, 8.f, 0.f)
                [
                    SNew(SButton)
                    .Text(NSLOCTEXT("RecipeBrowser", "NewRecipe", "New"))
                    .OnClicked(this, &SRecipeBrowserPanelImpl::OnCreateRecipe)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.f, 0.f, 8.f, 0.f)
                [
                    SNew(SButton)
                    .Text(NSLOCTEXT("RecipeBrowser", "DuplicateRecipe", "Duplicate"))
                    .OnClicked(this, &SRecipeBrowserPanelImpl::OnDuplicateRecipe)
                    .IsEnabled(this, &SRecipeBrowserPanelImpl::HasSelection)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.f, 0.f, 8.f, 0.f)
                [
                    SNew(SButton)
                    .Text(NSLOCTEXT("RecipeBrowser", "DeleteRecipe", "Delete"))
                    .OnClicked(this, &SRecipeBrowserPanelImpl::OnDeleteRecipe)
                    .IsEnabled(this, &SRecipeBrowserPanelImpl::HasSelection)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.f, 0.f, 8.f, 0.f)
                [
                    SNew(SButton)
                    .Text(NSLOCTEXT("RecipeBrowser", "ValidateRecipes", "Validate"))
                    .OnClicked(this, &SRecipeBrowserPanelImpl::OnValidateRecipes)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.f, 0.f, 8.f, 0.f)
                [
                    SNew(SButton)
                    .Text(NSLOCTEXT("RecipeBrowser", "ImportCSV", "Import CSV"))
                    .OnClicked(this, &SRecipeBrowserPanelImpl::OnImportCSV)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.f, 0.f, 8.f, 0.f)
                [
                    SNew(SButton)
                    .Text(NSLOCTEXT("RecipeBrowser", "ExportCSV", "Export CSV"))
                    .OnClicked(this, &SRecipeBrowserPanelImpl::OnExportCSV)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.f, 0.f, 8.f, 0.f)
                [
                    SNew(SButton)
                    .Text(NSLOCTEXT("RecipeBrowser", "SaveRecipe", "Save"))
                    .OnClicked(this, &SRecipeBrowserPanelImpl::OnSaveRecipe)
                    .IsEnabled(this, &SRecipeBrowserPanelImpl::HasSelection)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text(NSLOCTEXT("RecipeBrowser", "OpenLibrary", "Open Library"))
                    .OnClicked(this, &SRecipeBrowserPanelImpl::OnOpenLibrary)
                ];
        }

        bool HasSelection() const
        {
            return SelectedRecipe.IsValid();
        }

        FReply OnCreateRecipe()
        {
            FString PackagePath;
            FString AssetName;
            if (!PromptForNewAsset(PackagePath, AssetName))
            {
                return FReply::Handled();
            }

            if (UCraftingRecipe* NewAsset = URecipeEditorLibrary::CreateRecipeAsset(PackagePath, AssetName))
            {
                URecipeEditorLibrary::SaveAsset(NewAsset);
                RefreshRecipeList();
                SelectRecipe(NewAsset);
            }

            return FReply::Handled();
        }

        FReply OnDuplicateRecipe()
        {
            UCraftingRecipe* Current = SelectedRecipe.Get();
            if (!Current)
            {
                return FReply::Handled();
            }

            FString PackagePath;
            FString AssetName;
            if (!PromptForDuplicateAsset(Current, PackagePath, AssetName))
            {
                return FReply::Handled();
            }

            if (UCraftingRecipe* NewAsset = URecipeEditorLibrary::DuplicateRecipeAsset(Current, PackagePath, AssetName))
            {
                URecipeEditorLibrary::SaveAsset(NewAsset);
                RefreshRecipeList();
                SelectRecipe(NewAsset);
            }

            return FReply::Handled();
        }

        FReply OnDeleteRecipe()
        {
            UCraftingRecipe* Current = SelectedRecipe.Get();
            if (!Current)
            {
                return FReply::Handled();
            }

            const FText Message = FText::Format(NSLOCTEXT("RecipeBrowser", "ConfirmDelete", "Delete recipe '{0}'?"), FText::FromString(Current->GetName()));
            if (FMessageDialog::Open(EAppMsgType::YesNo, Message) != EAppReturnType::Yes)
            {
                return FReply::Handled();
            }

            const FString AssetPath = FPackageName::ObjectPathToPackageName(Current->GetPathName());
            if (UEditorAssetLibrary::DeleteAsset(AssetPath))
            {
                SelectedRecipe = nullptr;
                if (DetailsView.IsValid())
                {
                    DetailsView->SetObject(nullptr);
                }
                RefreshRecipeList();
            }

            return FReply::Handled();
        }

        FReply OnValidateRecipes()
        {
            TArray<FString> Errors;
            URecipeEditorLibrary::ValidateAllRecipes(Errors);

            if (Errors.Num() == 0)
            {
                FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("RecipeBrowser", "ValidationPassed", "All recipes are valid."));
            }
            else
            {
                FString Combined;
                for (const FString& Error : Errors)
                {
                    Combined += Error;
                    Combined += TEXT("\n");
                }
                FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Combined));
            }

            return FReply::Handled();
        }

        FReply OnImportCSV()
        {
            const FString PackageRoot = GetDefaultPackageRoot();

            TArray<FString> Files;
            if (!PromptForCSVFile(true, Files))
            {
                return FReply::Handled();
            }

            if (Files.Num() == 0)
            {
                return FReply::Handled();
            }

            TArray<UCraftingRecipe*> Updated;
            if (URecipeEditorLibrary::ImportRecipesFromCSV(Files[0], PackageRoot, Updated))
            {
                RefreshRecipeList();
                if (Updated.Num() > 0)
                {
                    SelectRecipe(Updated[0]);
                }
            }
            else
            {
                FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("RecipeBrowser", "ImportFailed", "Failed to import recipes from CSV."));
            }

            return FReply::Handled();
        }

        FReply OnExportCSV()
        {
            TArray<FString> Files;
            if (!PromptForCSVFile(false, Files))
            {
                return FReply::Handled();
            }

            if (Files.Num() == 0)
            {
                return FReply::Handled();
            }

            if (!URecipeEditorLibrary::ExportRecipesToCSV(Files[0]))
            {
                FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("RecipeBrowser", "ExportFailed", "Failed to export recipes to CSV."));
            }

            return FReply::Handled();
        }

        FReply OnSaveRecipe()
        {
            UCraftingRecipe* Current = SelectedRecipe.Get();
            if (Current)
            {
                URecipeEditorLibrary::SaveAsset(Current);
            }
            return FReply::Handled();
        }

        FReply OnOpenLibrary()
        {
            FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
            FARFilter Filter;
            Filter.ClassPaths.Add(UCraftingRecipeLibrary::StaticClass()->GetClassPathName());
            Filter.bRecursiveClasses = true;

            TArray<FAssetData> Libraries;
            AssetRegistryModule.Get().GetAssets(Filter, Libraries);

            if (Libraries.Num() == 0)
            {
                FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("RecipeBrowser", "NoLibraries", "No crafting recipe library assets were found."));
                return FReply::Handled();
            }

            FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
            ContentBrowserModule.Get().SyncBrowserToAssets(Libraries);

            if (GEditor)
            {
                if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
                {
                    for (const FAssetData& LibraryData : Libraries)
                    {
                        if (UObject* Asset = LibraryData.GetAsset())
                        {
                            AssetEditorSubsystem->OpenEditorForAsset(Asset);
                        }
                    }
                }
            }

            return FReply::Handled();
        }

        TSharedRef<ITableRow> GenerateRecipeRow(TSharedPtr<FCraftingRecipeListEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
        {
            return SNew(STableRow<TSharedPtr<FCraftingRecipeListEntry>>, OwnerTable)
                [
                    SNew(STextBlock)
                    .Text(Item->GetDisplayText())
                ];
        }

        void OnRecipeSelectionChanged(TSharedPtr<FCraftingRecipeListEntry> Item, ESelectInfo::Type SelectInfo)
        {
            if (Item.IsValid())
            {
                SelectedRecipe = Item->Recipe;
                if (DetailsView.IsValid())
                {
                    DetailsView->SetObject(SelectedRecipe.Get());
                }
            }
            else
            {
                SelectedRecipe = nullptr;
                if (DetailsView.IsValid())
                {
                    DetailsView->SetObject(nullptr);
                }
            }
        }

        void OnSearchTextChanged(const FText& InText)
        {
            SearchText = InText;
            RefreshRecipeList();
        }

        bool PromptForNewAsset(FString& OutPackagePath, FString& OutAssetName) const
        {
            FString DefaultPath = GetDefaultPackageRoot();
            FSaveAssetDialogConfig Config;
            Config.DefaultPath = DefaultPath;
            Config.DefaultAssetName = TEXT("NewRecipe");
#if (ENGINE_MAJOR_VERSION >= 5)
            Config.AssetClassNames.Add(UCraftingRecipe::StaticClass()->GetClassPathName());
#else
            Config.AssetClassNames.Add(UCraftingRecipe::StaticClass()->GetFName());
#endif
            Config.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

            FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
            const FString ObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(Config);
            if (ObjectPath.IsEmpty())
            {
                return false;
            }

            OutPackagePath = FPackageName::GetLongPackagePath(ObjectPath);
            OutAssetName = FPackageName::GetLongPackageAssetName(ObjectPath);
            return true;
        }

        bool PromptForDuplicateAsset(UCraftingRecipe* Source, FString& OutPackagePath, FString& OutAssetName) const
        {
            if (!Source)
            {
                return false;
            }

            FString DefaultPath = GetDefaultPackageRoot();
            FSaveAssetDialogConfig Config;
            Config.DefaultPath = DefaultPath;
            Config.DefaultAssetName = Source->GetName() + TEXT("_Copy");
#if (ENGINE_MAJOR_VERSION >= 5)
            Config.AssetClassNames.Add(UCraftingRecipe::StaticClass()->GetClassPathName());
#else
            Config.AssetClassNames.Add(UCraftingRecipe::StaticClass()->GetFName());
#endif
            Config.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

            FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
            const FString ObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(Config);
            if (ObjectPath.IsEmpty())
            {
                return false;
            }

            OutPackagePath = FPackageName::GetLongPackagePath(ObjectPath);
            OutAssetName = FPackageName::GetLongPackageAssetName(ObjectPath);
            return true;
        }

        bool PromptForCSVFile(bool bOpenDialog, TArray<FString>& OutFiles) const
        {
            IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
            if (!DesktopPlatform)
            {
                return false;
            }

            const void* ParentWindowHandle = nullptr;
            if (FSlateApplication::IsInitialized())
            {
                ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
            }

            const FString DialogTitle = bOpenDialog ? TEXT("Import Recipes From CSV") : TEXT("Export Recipes To CSV");
            const FString FileTypes = TEXT("CSV Files (*.csv)|*.csv");
            const FString DefaultPath = FPaths::ProjectDir();

            OutFiles.Reset();
            if (bOpenDialog)
            {
                return DesktopPlatform->OpenFileDialog(ParentWindowHandle, DialogTitle, DefaultPath, TEXT(""), FileTypes, EFileDialogFlags::None, OutFiles);
            }

            const bool bResult = DesktopPlatform->SaveFileDialog(ParentWindowHandle, DialogTitle, DefaultPath, TEXT("Recipes.csv"), FileTypes, EFileDialogFlags::None, OutFiles);
            return bResult && OutFiles.Num() > 0;
        }

        FString GetDefaultPackageRoot() const
        {
            if (SelectedRecipe.IsValid())
            {
                const FString PackageName = SelectedRecipe->GetOutermost()->GetName();
                return FPackageName::GetLongPackagePath(PackageName);
            }

            return DefaultRecipesRoot;
        }

    private:
        TWeakObjectPtr<UCraftingRecipe> SelectedRecipe;
        FText SearchText;
        TSharedPtr<SSearchBox> SearchBox;
        TArray<TSharedPtr<FCraftingRecipeListEntry>> RecipeEntries;
        TSharedPtr<SListView<TSharedPtr<FCraftingRecipeListEntry>>> RecipeListView;
        TSharedPtr<IDetailsView> DetailsView;
    };
}

using namespace RecipeBrowser;

void UWBP_RecipeBrowser::NativeConstruct()
{
    Super::NativeConstruct();

    RefreshRecipes();
}

void UWBP_RecipeBrowser::NativeDestruct()
{
    Super::NativeDestruct();
}

TSharedRef<SWidget> UWBP_RecipeBrowser::RebuildWidget()
{
    TSharedRef<SRecipeBrowserPanelImpl> Panel = SNew(SRecipeBrowserPanelImpl);
    RecipeBrowserPanel = Panel;
    return Panel;
}

void UWBP_RecipeBrowser::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    RecipeBrowserPanel.Reset();
}

void UWBP_RecipeBrowser::RefreshRecipes()
{
    if (RecipeBrowserPanel.IsValid())
    {
        if (TSharedPtr<SRecipeBrowserPanelImpl> Panel = StaticCastSharedPtr<SRecipeBrowserPanelImpl>(RecipeBrowserPanel))
        {
            Panel->RefreshRecipeList();
        }
    }
}

#endif // WITH_EDITOR
