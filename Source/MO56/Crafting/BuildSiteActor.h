#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildSiteActor.generated.h"

class UCraftingRecipe;
class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;
class UInventoryComponent;

using FBuildMaterialsMap = TMap<FName, int32>;

USTRUCT(BlueprintType)
struct MO56_API FBuildMaterialEntry
{
        GENERATED_BODY()

        FBuildMaterialEntry() = default;

        FBuildMaterialEntry(const FName& InItemId, int32 InRemaining)
                : ItemId(InItemId)
                , Remaining(InRemaining)
        {
        }

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FName ItemId = NAME_None;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int32 Remaining = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBuildSiteProgressUpdated, UCraftingRecipe*, Recipe, const TArray<FBuildMaterialEntry>&, MaterialsRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildSiteCompleted, UCraftingRecipe*, Recipe);

/**
 * Replicated build site actor spawned when a player places a buildable recipe in the world.
 *
 * Editor Implementation Guide:
 * 1. Create a Blueprint subclass (e.g. BP_BuildSiteActor) so designers can author per-recipe visuals.
 * 2. Set the BlueprintMesh component to the hologram mesh that should appear while construction is in progress.
 * 3. Tune the InteractionSphere radius/collision to match the distance players can contribute materials from.
 * 4. For each buildable recipe asset, set BuildableActorClass and BuildMaterialRequirements so InitializeFromRecipe has data.
 * 5. Expose OnBuildProgress / OnBuildCompleted in the Blueprint graph to trigger VFX, SFX, or UI notifications.
 */
UCLASS()
class MO56_API ABuildSiteActor : public AActor
{
        GENERATED_BODY()

public:
        ABuildSiteActor();

        virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

        /** Initializes the site state using the supplied recipe. */
        void InitializeFromRecipe(UCraftingRecipe* InRecipe);

        /** Attempts to contribute materials from the specified inventory. */
        UFUNCTION(BlueprintCallable, Category = "Build")
        bool ContributeMaterials(UInventoryComponent* Inventory);

        UFUNCTION(BlueprintPure, Category = "Build")
        bool IsComplete() const { return bCompleted; }

        UFUNCTION(BlueprintPure, Category = "Build")
        UCraftingRecipe* GetRecipe() const { return Recipe; }

        UFUNCTION(BlueprintPure, Category = "Build")
        TArray<FBuildMaterialEntry> GetMaterialsRemaining() const;

        /** Fired whenever the materials map changes. */
        UPROPERTY(BlueprintAssignable, Category = "Build")
        FOnBuildSiteProgressUpdated OnBuildProgress;

        /** Fired when the build is completed. */
        UPROPERTY(BlueprintAssignable, Category = "Build")
        FOnBuildSiteCompleted OnBuildCompleted;

protected:
        void RebuildMaterialsFromRecipe();
        void SpawnCompletedActor();
        void NotifyProgressChanged();

        bool PullMaterialFromInventory(UInventoryComponent& Inventory, const FName& ItemId, int32& CountRemaining);

protected:
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        TObjectPtr<USceneComponent> Root;

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        TObjectPtr<UStaticMeshComponent> BlueprintMesh;

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        TObjectPtr<USphereComponent> InteractionSphere;

        TMap<FName, int32> MaterialsRemaining;

        UPROPERTY(ReplicatedUsing = OnRep_Materials)
        TArray<FBuildMaterialEntry> ReplicatedMaterials;

        UPROPERTY(ReplicatedUsing = OnRep_Completed)
        bool bCompleted = false;

        UPROPERTY(Replicated)
        TObjectPtr<UCraftingRecipe> Recipe;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
        FName BlueprintFillParameter = TEXT("BuildFill");

        UFUNCTION()
        void OnRep_Materials();

        UFUNCTION()
        void OnRep_Completed();

        void UpdateReplicatedMaterialsFromMap();
        void RebuildMaterialsMapFromReplicatedData();
};
