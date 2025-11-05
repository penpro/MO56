#include "RecipeEditorLibrary.h"

#if WITH_EDITOR

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Crafting/CraftingRecipe.h"
#include "Crafting/CraftingRecipeLibrary.h"
#include "Crafting/CraftingRecipeRows.h"
#include "EditorAssetLibrary.h"
#include "Factories/DataAssetFactory.h"
#include "Serialization/Csv/CsvParser.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "UObject/SoftObjectPath.h"

namespace RecipeEditorLibrary
{
    namespace Private
    {
        static FString NormalizePackagePath(const FString& InPath)
        {
            FString Result = InPath;
            Result.TrimStartAndEndInline();
            if (Result.EndsWith(TEXT("/")))
            {
                Result.LeftChopInline(1);
            }

            if (!Result.StartsWith(TEXT("/")))
            {
                Result = TEXT("/") + Result;
            }

            return Result;
        }

        static FString BuildAssetPath(const FString& PackagePath, const FString& AssetName)
        {
            if (PackagePath.IsEmpty())
            {
                return FString();
            }

            const FString SanitizedName = ObjectTools::SanitizeObjectName(AssetName);
            if (SanitizedName.IsEmpty())
            {
                return FString();
            }
            FString NormalizedPath = NormalizePackagePath(PackagePath);
            if (!FPackageName::IsValidLongPackageName(NormalizedPath))
            {
                return FString();
            }
            return NormalizedPath + TEXT("/") + SanitizedName;
        }

        static TArray<FCraftingItemQty> ParseItemList(const FString& Source)
        {
            TArray<FCraftingItemQty> Result;

            TArray<FString> Pairs;
            Source.ParseIntoArray(Pairs, TEXT(";"), true);
            for (FString Pair : Pairs)
            {
                Pair.TrimStartAndEndInline();
                if (Pair.IsEmpty())
                {
                    continue;
                }

                FString NamePart;
                FString QuantityPart;
                if (!Pair.Split(TEXT(":"), &NamePart, &QuantityPart))
                {
                    continue;
                }

                NamePart.TrimStartAndEndInline();
                QuantityPart.TrimStartAndEndInline();

                const FName ItemName(*NamePart);
                if (ItemName.IsNone())
                {
                    continue;
                }

                const int32 Quantity = FCString::Atoi(*QuantityPart);
                if (Quantity <= 0)
                {
                    continue;
                }

                FCraftingItemQty& Entry = Result.AddDefaulted_GetRef();
                Entry.ItemId = ItemName;
                Entry.Quantity = Quantity;
            }

            return Result;
        }

        static FString ItemListToString(const TMap<FName, int32>& Source)
        {
            FString Result;
            bool bFirst = true;
            for (const TPair<FName, int32>& Pair : Source)
            {
                if (Pair.Key.IsNone() || Pair.Value <= 0)
                {
                    continue;
                }

                if (!bFirst)
                {
                    Result += TEXT(";");
                }
                bFirst = false;

                Result += Pair.Key.ToString();
                Result += TEXT(":");
                Result += FString::FromInt(Pair.Value);
            }

            return Result;
        }

        static TMap<FName, int32> ToItemMap(const TArray<FCraftingItemQty>& Items)
        {
            TMap<FName, int32> Result;
            for (const FCraftingItemQty& Item : Items)
            {
                if (!Item.ItemId.IsNone() && Item.Quantity > 0)
                {
                    Result.Add(Item.ItemId, Item.Quantity);
                }
            }
            return Result;
        }

        static FString EscapeCsv(const FString& Value)
        {
            FString Result = Value;
            Result.ReplaceInline(TEXT("\r"), TEXT(" "));
            Result.ReplaceInline(TEXT("\n"), TEXT(" "));

            const bool bNeedsQuotes = Result.Contains(TEXT(",")) || Result.Contains(TEXT("\""));
            Result.ReplaceInline(TEXT("\""), TEXT("\"\""));
            if (bNeedsQuotes)
            {
                Result = FString::Printf(TEXT("\"%s\""), *Result);
            }

            return Result;
        }

        static TSubclassOf<AActor> LoadBuildableClass(const TSoftClassPtr<AActor>& SoftClass)
        {
            return SoftClass.IsNull() ? nullptr : SoftClass.LoadSynchronous();
        }
    }
}

void URecipeEditorLibrary::ListAllRecipeAssets(TArray<UCraftingRecipe*>& OutAssets)
{
    OutAssets.Reset();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    FARFilter Filter;
    Filter.ClassPaths.Add(UCraftingRecipe::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;

    TArray<FAssetData> AssetData;
    AssetRegistryModule.Get().GetAssets(Filter, AssetData);
    for (const FAssetData& Data : AssetData)
    {
        if (UCraftingRecipe* Recipe = Cast<UCraftingRecipe>(Data.GetAsset()))
        {
            OutAssets.Add(Recipe);
        }
    }
}

UCraftingRecipe* URecipeEditorLibrary::CreateRecipeAsset(const FString& PackagePath, const FString& AssetName)
{
    if (PackagePath.IsEmpty() || AssetName.IsEmpty())
    {
        return nullptr;
    }

    const FString SanitizedName = ObjectTools::SanitizeObjectName(AssetName);
    if (SanitizedName.IsEmpty())
    {
        return nullptr;
    }
    const FString NormalizedPath = RecipeEditorLibrary::Private::NormalizePackagePath(PackagePath);
    if (!FPackageName::IsValidLongPackageName(NormalizedPath))
    {
        return nullptr;
    }

    UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
    Factory->DataAssetClass = UCraftingRecipe::StaticClass();

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    UObject* NewAsset = AssetToolsModule.Get().CreateAsset(SanitizedName, NormalizedPath, UCraftingRecipe::StaticClass(), Factory);
    return Cast<UCraftingRecipe>(NewAsset);
}

UCraftingRecipe* URecipeEditorLibrary::DuplicateRecipeAsset(UCraftingRecipe* SourceAsset, const FString& PackagePath, const FString& NewName)
{
    if (!SourceAsset || PackagePath.IsEmpty() || NewName.IsEmpty())
    {
        return nullptr;
    }

    const FString DestinationPath = RecipeEditorLibrary::Private::BuildAssetPath(PackagePath, NewName);
    if (DestinationPath.IsEmpty())
    {
        return nullptr;
    }

    const FString SourceObjectPath = SourceAsset->GetPathName();
    const FString SourceAssetPath = FPackageName::ObjectPathToPackageName(SourceObjectPath);
    UObject* Duplicated = UEditorAssetLibrary::DuplicateAsset(SourceAssetPath, DestinationPath);
    return Cast<UCraftingRecipe>(Duplicated);
}

bool URecipeEditorLibrary::UpdateRecipeAssetFromRow(UCraftingRecipe* Target, const FCraftingRecipeRow& Row)
{
    if (!Target)
    {
        return false;
    }

    bool bChanged = false;

    if (!Target->DisplayName.EqualTo(Row.DisplayName))
    {
        Target->DisplayName = Row.DisplayName;
        bChanged = true;
    }

    if (Target->RequiredKnowledge != Row.RequiredKnowledge)
    {
        Target->RequiredKnowledge = Row.RequiredKnowledge;
        bChanged = true;
    }

    if (Target->PrimarySkillTag != Row.PrimarySkillTag)
    {
        Target->PrimarySkillTag = Row.PrimarySkillTag;
        bChanged = true;
    }

    if (!FMath::IsNearlyEqual(Target->BaseDuration, Row.BaseDuration))
    {
        Target->BaseDuration = Row.BaseDuration;
        bChanged = true;
    }

    if (!FMath::IsNearlyEqual(Target->BaseDifficulty, static_cast<float>(Row.BaseDifficulty)))
    {
        Target->BaseDifficulty = static_cast<float>(Row.BaseDifficulty);
        bChanged = true;
    }

    if (!FMath::IsNearlyEqual(Target->KnowledgeOnTry, Row.KnowledgeOnTry))
    {
        Target->KnowledgeOnTry = Row.KnowledgeOnTry;
        bChanged = true;
    }

    if (!FMath::IsNearlyEqual(Target->KnowledgeOnSuccess, Row.KnowledgeOnSuccess))
    {
        Target->KnowledgeOnSuccess = Row.KnowledgeOnSuccess;
        bChanged = true;
    }

    if (Target->bIsBuildable != Row.bIsBuildable)
    {
        Target->bIsBuildable = Row.bIsBuildable;
        bChanged = true;
    }

    const TSubclassOf<AActor> NewBuildableClass = RecipeEditorLibrary::Private::LoadBuildableClass(Row.BuildableActorClass);
    if (Target->BuildableActorClass != NewBuildableClass)
    {
        Target->BuildableActorClass = NewBuildableClass;
        bChanged = true;
    }

    const TMap<FName, int32> NewInputs = RecipeEditorLibrary::Private::ToItemMap(Row.Inputs);
    if (Target->Inputs.OrderIndependentCompareEqual(NewInputs) == false)
    {
        Target->Inputs = NewInputs;
        bChanged = true;
    }

    const TMap<FName, int32> NewOutputs = RecipeEditorLibrary::Private::ToItemMap(Row.Outputs);
    if (Target->Outputs.OrderIndependentCompareEqual(NewOutputs) == false)
    {
        Target->Outputs = NewOutputs;
        bChanged = true;
    }

    const TMap<FName, int32> NewFailByproducts = RecipeEditorLibrary::Private::ToItemMap(Row.FailByproducts);
    if (Target->FailByproducts.OrderIndependentCompareEqual(NewFailByproducts) == false)
    {
        Target->FailByproducts = NewFailByproducts;
        bChanged = true;
    }

    const TMap<FName, int32> NewBuildMaterials = Row.bIsBuildable
                                                    ? RecipeEditorLibrary::Private::ToItemMap(Row.BuildMaterialRequirements)
                                                    : TMap<FName, int32>();
    if (Target->BuildMaterialRequirements.OrderIndependentCompareEqual(NewBuildMaterials) == false)
    {
        Target->BuildMaterialRequirements = NewBuildMaterials;
        bChanged = true;
    }

    if (bChanged)
    {
        Target->MarkPackageDirty();
    }

    return bChanged;
}

bool URecipeEditorLibrary::SaveAsset(UObject* Asset)
{
    if (!Asset)
    {
        return false;
    }

    const FString ObjectPath = Asset->GetPathName();
    const FString AssetPath = FPackageName::ObjectPathToPackageName(ObjectPath);
    return UEditorAssetLibrary::SaveAsset(AssetPath);
}

bool URecipeEditorLibrary::SaveLibrary(UCraftingRecipeLibrary* Library)
{
    return SaveAsset(Library);
}

bool URecipeEditorLibrary::ImportRecipesFromCSV(const FString& CSVPath, const FString& PackageRoot, TArray<UCraftingRecipe*>& OutCreatedOrUpdated)
{
    OutCreatedOrUpdated.Reset();

    FString CSVContent;
    if (!FFileHelper::LoadFileToString(CSVContent, *CSVPath))
    {
        return false;
    }

    const FCsvParser Parser(CSVContent);
    const auto& Rows = Parser.GetRows();
    if (Rows.Num() <= 1)
    {
        return false;
    }

    TMap<FString, int32> ColumnLookup;
    const TArray<const TCHAR*>& Header = Rows[0];
    for (int32 ColumnIndex = 0; ColumnIndex < Header.Num(); ++ColumnIndex)
    {
        FString ColumnName(Header[ColumnIndex]);
        ColumnName.TrimStartAndEndInline();
        ColumnLookup.Add(ColumnName, ColumnIndex);
    }

    auto GetCell = [&ColumnLookup](const TArray<const TCHAR*>& Cells, const FString& ColumnName) -> FString
    {
        if (const int32* Index = ColumnLookup.Find(ColumnName))
        {
            if (Cells.IsValidIndex(*Index))
            {
                FString Value(Cells[*Index]);
                Value.TrimStartAndEndInline();
                return Value;
            }
        }
        return FString();
    };

    for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
    {
        const TArray<const TCHAR*>& Cells = Rows[RowIndex];
        if (Cells.Num() == 0)
        {
            continue;
        }

        FCraftingRecipeRow RowStruct;
        RowStruct.RecipeId = FName(*GetCell(Cells, TEXT("RecipeId")));
        if (RowStruct.RecipeId.IsNone())
        {
            continue;
        }

        RowStruct.DisplayName = FText::FromString(GetCell(Cells, TEXT("DisplayName")));
        RowStruct.RequiredKnowledge = FName(*GetCell(Cells, TEXT("RequiredKnowledge")));
        RowStruct.PrimarySkillTag = FName(*GetCell(Cells, TEXT("PrimarySkillTag")));

        const FString BaseDurationString = GetCell(Cells, TEXT("BaseDuration"));
        if (!BaseDurationString.IsEmpty())
        {
            RowStruct.BaseDuration = FCString::Atof(*BaseDurationString);
        }

        const FString BaseDifficultyString = GetCell(Cells, TEXT("BaseDifficulty"));
        if (!BaseDifficultyString.IsEmpty())
        {
            RowStruct.BaseDifficulty = FCString::Atoi(*BaseDifficultyString);
        }

        const FString KnowledgeOnTryString = GetCell(Cells, TEXT("KnowledgeOnTry"));
        if (!KnowledgeOnTryString.IsEmpty())
        {
            RowStruct.KnowledgeOnTry = FCString::Atof(*KnowledgeOnTryString);
        }

        const FString KnowledgeOnSuccessString = GetCell(Cells, TEXT("KnowledgeOnSuccess"));
        if (!KnowledgeOnSuccessString.IsEmpty())
        {
            RowStruct.KnowledgeOnSuccess = FCString::Atof(*KnowledgeOnSuccessString);
        }

        RowStruct.bIsBuildable = GetCell(Cells, TEXT("bIsBuildable")).Equals(TEXT("true"), ESearchCase::IgnoreCase);

        const FString BuildablePath = GetCell(Cells, TEXT("BuildableActorPath"));
        if (!BuildablePath.IsEmpty())
        {
            RowStruct.BuildableActorClass = TSoftClassPtr<AActor>(FSoftClassPath(BuildablePath));
        }
        else
        {
            RowStruct.BuildableActorClass.Reset();
        }

        RowStruct.Inputs = RecipeEditorLibrary::Private::ParseItemList(GetCell(Cells, TEXT("Inputs")));
        RowStruct.Outputs = RecipeEditorLibrary::Private::ParseItemList(GetCell(Cells, TEXT("Outputs")));
        RowStruct.FailByproducts = RecipeEditorLibrary::Private::ParseItemList(GetCell(Cells, TEXT("FailByproducts")));
        RowStruct.BuildMaterialRequirements = RecipeEditorLibrary::Private::ParseItemList(GetCell(Cells, TEXT("BuildMaterials")));

        const FString AssetName = RowStruct.RecipeId.ToString();
        const FString AssetPath = RecipeEditorLibrary::Private::BuildAssetPath(PackageRoot, AssetName);
        if (AssetPath.IsEmpty())
        {
            continue;
        }

        const bool bAssetExists = UEditorAssetLibrary::DoesAssetExist(AssetPath);

        UCraftingRecipe* TargetAsset = nullptr;
        if (bAssetExists)
        {
            if (UObject* Loaded = UEditorAssetLibrary::LoadAsset(AssetPath))
            {
                TargetAsset = Cast<UCraftingRecipe>(Loaded);
            }
        }
        else
        {
            TargetAsset = CreateRecipeAsset(PackageRoot, AssetName);
        }

        if (!TargetAsset)
        {
            continue;
        }

        const bool bAssetChanged = UpdateRecipeAssetFromRow(TargetAsset, RowStruct);
        if (bAssetChanged)
        {
            SaveAsset(TargetAsset);
        }
        if (!bAssetExists || bAssetChanged)
        {
            OutCreatedOrUpdated.AddUnique(TargetAsset);
        }
    }

    return OutCreatedOrUpdated.Num() > 0;
}

bool URecipeEditorLibrary::ExportRecipesToCSV(const FString& CSVPath)
{
    TArray<UCraftingRecipe*> Recipes;
    ListAllRecipeAssets(Recipes);
    if (Recipes.Num() == 0)
    {
        return false;
    }

    FString Output = TEXT("RecipeId,DisplayName,RequiredKnowledge,PrimarySkillTag,BaseDuration,BaseDifficulty,KnowledgeOnTry,KnowledgeOnSuccess,bIsBuildable,BuildableActorPath,Inputs,Outputs,FailByproducts,BuildMaterials\n");

    for (UCraftingRecipe* Recipe : Recipes)
    {
        if (!Recipe)
        {
            continue;
        }

        const FString RecipeId = Recipe->GetName();
        const FString DisplayName = Recipe->DisplayName.ToString();
        const FString RequiredKnowledge = Recipe->RequiredKnowledge.ToString();
        const FString PrimarySkillTag = Recipe->PrimarySkillTag.ToString();
        const FString BaseDuration = FString::SanitizeFloat(Recipe->BaseDuration);
        const FString BaseDifficulty = FString::SanitizeFloat(Recipe->BaseDifficulty);
        const FString KnowledgeOnTry = FString::SanitizeFloat(Recipe->KnowledgeOnTry);
        const FString KnowledgeOnSuccess = FString::SanitizeFloat(Recipe->KnowledgeOnSuccess);
        const FString bIsBuildable = Recipe->bIsBuildable ? TEXT("true") : TEXT("false");
        const FString BuildablePath = Recipe->BuildableActorClass ? Recipe->BuildableActorClass->GetPathName() : FString();
        const FString Inputs = RecipeEditorLibrary::Private::ItemListToString(Recipe->Inputs);
        const FString Outputs = RecipeEditorLibrary::Private::ItemListToString(Recipe->Outputs);
        const FString FailByproducts = RecipeEditorLibrary::Private::ItemListToString(Recipe->FailByproducts);
        const FString BuildMaterials = Recipe->BuildMaterialRequirements.Num() > 0 ? RecipeEditorLibrary::Private::ItemListToString(Recipe->BuildMaterialRequirements) : FString();

        Output += RecipeEditorLibrary::Private::EscapeCsv(RecipeId) + TEXT(",") +
                  RecipeEditorLibrary::Private::EscapeCsv(DisplayName) + TEXT(",") +
                  RecipeEditorLibrary::Private::EscapeCsv(RequiredKnowledge) + TEXT(",") +
                  RecipeEditorLibrary::Private::EscapeCsv(PrimarySkillTag) + TEXT(",") +
                  BaseDuration + TEXT(",") +
                  BaseDifficulty + TEXT(",") +
                  KnowledgeOnTry + TEXT(",") +
                  KnowledgeOnSuccess + TEXT(",") +
                  RecipeEditorLibrary::Private::EscapeCsv(bIsBuildable) + TEXT(",") +
                  RecipeEditorLibrary::Private::EscapeCsv(BuildablePath) + TEXT(",") +
                  RecipeEditorLibrary::Private::EscapeCsv(Inputs) + TEXT(",") +
                  RecipeEditorLibrary::Private::EscapeCsv(Outputs) + TEXT(",") +
                  RecipeEditorLibrary::Private::EscapeCsv(FailByproducts) + TEXT(",") +
                  RecipeEditorLibrary::Private::EscapeCsv(BuildMaterials) + TEXT("\n");
    }

    return FFileHelper::SaveStringToFile(Output, *CSVPath);
}

void URecipeEditorLibrary::ValidateAllRecipes(TArray<FString>& OutErrors)
{
    OutErrors.Reset();

    TArray<UCraftingRecipe*> Recipes;
    ListAllRecipeAssets(Recipes);

    for (UCraftingRecipe* Recipe : Recipes)
    {
        if (!Recipe)
        {
            continue;
        }

        const FString RecipeId = Recipe->GetName();

        if (Recipe->DisplayName.IsEmptyOrWhitespace())
        {
            OutErrors.Add(FString::Printf(TEXT("%s: DisplayName is empty"), *RecipeId));
        }

        if (Recipe->Inputs.Num() == 0)
        {
            OutErrors.Add(FString::Printf(TEXT("%s: Inputs are empty"), *RecipeId));
        }

        if (Recipe->Outputs.Num() == 0 && !Recipe->bIsBuildable)
        {
            OutErrors.Add(FString::Printf(TEXT("%s: Outputs are empty"), *RecipeId));
        }

        if (Recipe->bIsBuildable)
        {
            if (!Recipe->BuildableActorClass)
            {
                OutErrors.Add(FString::Printf(TEXT("%s: Buildable recipe missing BuildableActorClass"), *RecipeId));
            }

            if (Recipe->BuildMaterialRequirements.Num() == 0)
            {
                OutErrors.Add(FString::Printf(TEXT("%s: Buildable recipe missing BuildMaterialRequirements"), *RecipeId));
            }
        }
    }
}

#endif // WITH_EDITOR
