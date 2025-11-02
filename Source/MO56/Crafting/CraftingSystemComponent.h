#pragma once

#include "Components/ActorComponent.h"
#include "CraftingSystemComponent.generated.h"

class UCraftingRecipe;
class UInventoryComponent;
class USkillSystemComponent;
class UItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftStarted, FText, Name, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftProgress, float, Elapsed, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftFinished, UCraftingRecipe*, Recipe, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftCancelled, FName, Reason);

/** Reservation entry tracked while a craft is active. */
USTRUCT(BlueprintType)
struct FCraftingReservation
{
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
        FName ItemId = NAME_None;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
        int32 Quantity = 0;
};

USTRUCT()
struct FCraftingActionState
{
        GENERATED_BODY()

        UPROPERTY()
        TObjectPtr<UCraftingRecipe> Recipe = nullptr;

        UPROPERTY()
        float StartTime = 0.f;

        UPROPERTY()
        float Duration = 0.f;

        UPROPERTY()
        TArray<FCraftingReservation> Reservations;

        bool IsActive() const { return Recipe != nullptr; }

        void Reset()
        {
                Recipe = nullptr;
                StartTime = 0.f;
                Duration = 0.f;
                Reservations.Reset();
        }
};

/**
 * Component that coordinates crafting attempts and communicates with inventory/skills.
 *
 * Editor Implementation Guide:
 * 1. Add UCraftingSystemComponent to the character Blueprint (Defaults > Components) alongside Inventory/Skill components.
 * 2. Populate InitialRecipes/InitialBuildRecipes arrays with CraftingRecipe assets the player should know at spawn.
 * 3. Bind OnCraftStarted/Progress/Finished/Cancelled delegates in Blueprint to drive UI timers, audio, and reward flows.
 * 4. Expose TryCraftRecipe/CancelCraft through input handlers or widgets so players can trigger crafting from menus.
 * 5. When saving, call WriteToSaveData/ReadFromSaveData via the save subsystem to persist unlocked recipes and progress.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MO56_API UCraftingSystemComponent : public UActorComponent
{
        GENERATED_BODY()

public:
        UCraftingSystemComponent();

        virtual void BeginPlay() override;
        virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
        virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

        /** Returns the recipes currently known that can be crafted with the available knowledge and inventory. */
        UFUNCTION(BlueprintCallable, Category = "Crafting")
        void GetCraftableRecipes(TArray<UCraftingRecipe*>& OutRecipes) const;

        /** Attempts to begin crafting the supplied recipe. Returns true when the request was accepted. */
        UFUNCTION(BlueprintCallable, Category = "Crafting")
        bool TryCraftRecipe(UCraftingRecipe* Recipe);

        /** Cancels any active craft, returning reserved inputs to the inventory. */
        UFUNCTION(BlueprintCallable, Category = "Crafting")
        void CancelCraft(FName Reason);

        UFUNCTION(BlueprintPure, Category = "Crafting")
        bool HasActiveCraft() const { return ActiveCraft.IsActive(); }

        /** Known non-build recipes unlocked for this component. */
        UFUNCTION(BlueprintCallable, Category = "Crafting")
        const TArray<UCraftingRecipe*>& GetKnownRecipes() const { return KnownRecipes; }

        /** Known build recipes for placement in build mode. */
        UFUNCTION(BlueprintCallable, Category = "Crafting")
        const TArray<UCraftingRecipe*>& GetKnownBuildRecipes() const { return KnownBuildRecipes; }

        /** Adds a recipe to the known list if not already present. */
        UFUNCTION(BlueprintCallable, Category = "Crafting")
        void UnlockRecipe(UCraftingRecipe* Recipe);

        /** Adds a build recipe to the known list if not already present. */
        UFUNCTION(BlueprintCallable, Category = "Crafting")
        void UnlockBuildRecipe(UCraftingRecipe* Recipe);

        /** Broadcast when a craft begins. */
        UPROPERTY(BlueprintAssignable, Category = "Crafting")
        FOnCraftStarted OnCraftStarted;

        /** Broadcast while an active craft progresses. */
        UPROPERTY(BlueprintAssignable, Category = "Crafting")
        FOnCraftProgress OnCraftProgress;

        /** Broadcast when a craft finishes. */
        UPROPERTY(BlueprintAssignable, Category = "Crafting")
        FOnCraftFinished OnCraftFinished;

        /** Broadcast when a craft is cancelled. */
        UPROPERTY(BlueprintAssignable, Category = "Crafting")
        FOnCraftCancelled OnCraftCancelled;

protected:
        UFUNCTION(Server, Reliable)
        void ServerTryCraftRecipe(UCraftingRecipe* Recipe);

        UFUNCTION(Server, Reliable)
        void ServerCancelCraft(FName Reason);

        UFUNCTION(Client, Reliable)
        void ClientCraftStarted(UCraftingRecipe* Recipe, float Duration);

        UFUNCTION(Client, Reliable)
        void ClientCraftFinished(UCraftingRecipe* Recipe, bool bWasSuccessful);

        UFUNCTION(Client, Reliable)
        void ClientCraftCancelled(FName Reason);

        UFUNCTION()
        void HandleCraftTimerFinished();

        void StartCraftInternal(UCraftingRecipe* Recipe);
        void FinishCraftInternal(bool bWasSuccessful);
        bool CanCraftRecipe(const UCraftingRecipe& Recipe) const;
        bool HasRequiredKnowledge(const UCraftingRecipe& Recipe) const;
        bool HasRequiredInputs(const UCraftingRecipe& Recipe) const;
        void ReserveInputs(const UCraftingRecipe& Recipe, TArray<FCraftingReservation>& OutReservations) const;
        void ConsumeReservations(const TArray<FCraftingReservation>& Reservations);
        void ReturnReservations(const TArray<FCraftingReservation>& Reservations);
        void GrantOutputs(const UCraftingRecipe& Recipe, bool bWasSuccessful);
        float CalculateRecipeDuration(const UCraftingRecipe& Recipe) const;
        bool EvaluateCraftSuccess(const UCraftingRecipe& Recipe) const;

        UInventoryComponent* ResolveInventoryComponent() const;
        USkillSystemComponent* ResolveSkillSystem() const;
        UItemData* ResolveItemData(const FName& ItemId) const;
        int32 CountItemInInventory(const FName& ItemId) const;

        void BroadcastCraftStarted(const UCraftingRecipe& Recipe, float Duration);
        void BroadcastCraftFinished(UCraftingRecipe* Recipe, bool bWasSuccessful);
        void BroadcastCraftCancelled(const FName& Reason);

        void UpdateProgressDelegates();

protected:
        /** Recipes granted on spawn before any knowledge unlock checks. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
        TArray<TObjectPtr<UCraftingRecipe>> InitialRecipes;

        /** Build recipes granted on spawn. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
        TArray<TObjectPtr<UCraftingRecipe>> InitialBuildRecipes;

        UPROPERTY(Replicated)
        TArray<TObjectPtr<UCraftingRecipe>> KnownRecipes;

        UPROPERTY(Replicated)
        TArray<TObjectPtr<UCraftingRecipe>> KnownBuildRecipes;

        UPROPERTY(ReplicatedUsing = OnRep_ActiveCraft)
        FCraftingActionState ActiveCraft;

        UFUNCTION()
        void OnRep_ActiveCraft();

        /** Cached timer handle that completes the craft on the authority. */
        FTimerHandle ActiveCraftTimerHandle;

        /** Soft reference to the inventory component on the owning actor. */
        mutable TWeakObjectPtr<UInventoryComponent> CachedInventory;

        /** Soft reference to the skill system component on the owning actor. */
        mutable TWeakObjectPtr<USkillSystemComponent> CachedSkillSystem;
};
