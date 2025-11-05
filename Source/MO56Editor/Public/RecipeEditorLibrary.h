#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RecipeEditorLibrary.generated.h"

class UCraftingRecipe;
class UCraftingRecipeLibrary;
struct FCraftingRecipeRow;

UCLASS()
class MO56EDITOR_API URecipeEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Crafting|Editor")
    static void ListAllRecipeAssets(TArray<UCraftingRecipe*>& OutAssets);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Crafting|Editor")
    static UCraftingRecipe* CreateRecipeAsset(const FString& PackagePath, const FString& AssetName);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Crafting|Editor")
    static UCraftingRecipe* DuplicateRecipeAsset(UCraftingRecipe* SourceAsset, const FString& PackagePath, const FString& NewName);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Crafting|Editor")
    static bool UpdateRecipeAssetFromRow(UCraftingRecipe* Target, const FCraftingRecipeRow& Row);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Crafting|Editor")
    static bool SaveAsset(UObject* Asset);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Crafting|Editor")
    static bool SaveLibrary(UCraftingRecipeLibrary* Library);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Crafting|Editor")
    static bool ImportRecipesFromCSV(const FString& CSVPath, const FString& PackageRoot, TArray<UCraftingRecipe*>& OutCreatedOrUpdated);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Crafting|Editor")
    static bool ExportRecipesToCSV(const FString& CSVPath);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Crafting|Editor")
    static void ValidateAllRecipes(TArray<FString>& OutErrors);
#endif
};
