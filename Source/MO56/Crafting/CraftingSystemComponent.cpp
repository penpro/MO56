#include "Crafting/CraftingSystemComponent.h"

#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include "Crafting/CraftingRecipe.h"
#include "Skills/SkillSystemComponent.h"
#include "Skills/SkillTypes.h"

#include "ItemData.h"

namespace
{
        constexpr float KnowledgeUnlockThreshold = 1.f;
}

UCraftingSystemComponent::UCraftingSystemComponent()
{
        PrimaryComponentTick.bCanEverTick = true;
        SetIsReplicatedByDefault(true);
}

void UCraftingSystemComponent::BeginPlay()
{
        Super::BeginPlay();

        if (HasAuthority())
        {
                for (UCraftingRecipe* Recipe : InitialRecipes)
                {
                        UnlockRecipe(Recipe);
                }

                for (UCraftingRecipe* Recipe : InitialBuildRecipes)
                {
                        UnlockBuildRecipe(Recipe);
                }
        }
}

void UCraftingSystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);

        DOREPLIFETIME(UCraftingSystemComponent, KnownRecipes);
        DOREPLIFETIME(UCraftingSystemComponent, KnownBuildRecipes);
        DOREPLIFETIME(UCraftingSystemComponent, ActiveCraft);
}

void UCraftingSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (ActiveCraft.IsActive())
        {
                UpdateProgressDelegates();
        }
}

void UCraftingSystemComponent::GetCraftableRecipes(TArray<UCraftingRecipe*>& OutRecipes) const
{
        OutRecipes.Reset();

        for (UCraftingRecipe* Recipe : KnownRecipes)
        {
                        if (!Recipe)
                        {
                                continue;
                        }

                        if (CanCraftRecipe(*Recipe))
                        {
                                OutRecipes.Add(Recipe);
                        }
        }
}

bool UCraftingSystemComponent::TryCraftRecipe(UCraftingRecipe* Recipe)
{
        if (!Recipe)
        {
                return false;
        }

        if (AActor* Owner = GetOwner())
        {
                if (!Owner->HasAuthority())
                {
                        ServerTryCraftRecipe(Recipe);
                        return true;
                }
        }

        if (ActiveCraft.IsActive())
        {
                return false;
        }

        if (!CanCraftRecipe(*Recipe))
        {
                return false;
        }

        StartCraftInternal(Recipe);
        return true;
}

void UCraftingSystemComponent::CancelCraft(FName Reason)
{
        if (AActor* Owner = GetOwner())
        {
                if (!Owner->HasAuthority())
                {
                        ServerCancelCraft(Reason);
                        return;
                }
        }

        if (!ActiveCraft.IsActive())
        {
                return;
        }

        GetWorld()->GetTimerManager().ClearTimer(ActiveCraftTimerHandle);
        ReturnReservations(ActiveCraft.Reservations);
        ActiveCraft.Reset();
        BroadcastCraftCancelled(Reason);
        ClientCraftCancelled(Reason);
}

void UCraftingSystemComponent::UnlockRecipe(UCraftingRecipe* Recipe)
{
        if (!Recipe)
        {
                return;
        }

        if (KnownRecipes.Contains(Recipe))
        {
                return;
        }

        KnownRecipes.Add(Recipe);
}

void UCraftingSystemComponent::UnlockBuildRecipe(UCraftingRecipe* Recipe)
{
        if (!Recipe)
        {
                return;
        }

        if (KnownBuildRecipes.Contains(Recipe))
        {
                return;
        }

        KnownBuildRecipes.Add(Recipe);
}

void UCraftingSystemComponent::ServerTryCraftRecipe_Implementation(UCraftingRecipe* Recipe)
{
        TryCraftRecipe(Recipe);
}

void UCraftingSystemComponent::ServerCancelCraft_Implementation(FName Reason)
{
        CancelCraft(Reason);
}

void UCraftingSystemComponent::HandleCraftTimerFinished()
{
        if (!ActiveCraft.IsActive())
        {
                return;
        }

        const bool bWasSuccessful = EvaluateCraftSuccess(*ActiveCraft.Recipe);
        FinishCraftInternal(bWasSuccessful);
}

void UCraftingSystemComponent::StartCraftInternal(UCraftingRecipe* Recipe)
{
        if (!Recipe)
        {
                return;
        }

        ActiveCraft.Recipe = Recipe;
        ActiveCraft.StartTime = GetWorld()->GetTimeSeconds();
        ActiveCraft.Duration = CalculateRecipeDuration(*Recipe);
        ReserveInputs(*Recipe, ActiveCraft.Reservations);

        GetWorld()->GetTimerManager().SetTimer(ActiveCraftTimerHandle, this, &UCraftingSystemComponent::HandleCraftTimerFinished,
                                              ActiveCraft.Duration, false);

        BroadcastCraftStarted(*Recipe, ActiveCraft.Duration);
        ClientCraftStarted(Recipe, ActiveCraft.Duration);
}

void UCraftingSystemComponent::FinishCraftInternal(bool bWasSuccessful)
{
        if (!ActiveCraft.IsActive())
        {
                return;
        }

        ConsumeReservations(ActiveCraft.Reservations);
        GrantOutputs(*ActiveCraft.Recipe, bWasSuccessful);
        BroadcastCraftFinished(ActiveCraft.Recipe, bWasSuccessful);
        ClientCraftFinished(ActiveCraft.Recipe, bWasSuccessful);
        ActiveCraft.Reset();
}

bool UCraftingSystemComponent::CanCraftRecipe(const UCraftingRecipe& Recipe) const
{
        return HasRequiredKnowledge(Recipe) && HasRequiredInputs(Recipe);
}

bool UCraftingSystemComponent::HasRequiredKnowledge(const UCraftingRecipe& Recipe) const
{
        if (Recipe.RequiredKnowledge.IsNone())
        {
                return true;
        }

        if (const USkillSystemComponent* SkillSystem = ResolveSkillSystem())
        {
                return SkillSystem->GetKnowledgeValue(Recipe.RequiredKnowledge) >= KnowledgeUnlockThreshold;
        }

        return false;
}

bool UCraftingSystemComponent::HasRequiredInputs(const UCraftingRecipe& Recipe) const
{
        for (const TPair<FName, int32>& Entry : Recipe.Inputs)
        {
                if (Entry.Value <= 0)
                {
                        continue;
                }

                if (CountItemInInventory(Entry.Key) < Entry.Value)
                {
                        return false;
                }
        }

        return true;
}

void UCraftingSystemComponent::ReserveInputs(const UCraftingRecipe& Recipe, TArray<FCraftingReservation>& OutReservations) const
{
        OutReservations.Reset();

        for (const TPair<FName, int32>& Entry : Recipe.Inputs)
        {
                if (Entry.Value <= 0)
                {
                        continue;
                }

                FCraftingReservation& Reservation = OutReservations.AddDefaulted_GetRef();
                Reservation.ItemId = Entry.Key;
                Reservation.Quantity = Entry.Value;
        }
}

void UCraftingSystemComponent::ConsumeReservations(const TArray<FCraftingReservation>& Reservations)
{
        if (UInventoryComponent* Inventory = ResolveInventoryComponent())
        {
                for (const FCraftingReservation& Reservation : Reservations)
                {
                        if (Reservation.Quantity <= 0 || Reservation.ItemId.IsNone())
                        {
                                continue;
                        }

                        if (UItemData* ItemData = ResolveItemData(Reservation.ItemId))
                        {
                                Inventory->RemoveItem(ItemData, Reservation.Quantity);
                        }
                }
        }
}

void UCraftingSystemComponent::ReturnReservations(const TArray<FCraftingReservation>& Reservations)
{
        // Returning a reservation is a no-op because the items were never removed; the method is
        // provided for parity with future inventory reservation features.
        (void)Reservations;
}

void UCraftingSystemComponent::GrantOutputs(const UCraftingRecipe& Recipe, bool bWasSuccessful)
{
        UInventoryComponent* Inventory = ResolveInventoryComponent();
        if (!Inventory)
        {
                return;
        }

        const TMap<FName, int32>& Results = bWasSuccessful ? Recipe.Outputs : Recipe.FailByproducts;
        for (const TPair<FName, int32>& Entry : Results)
        {
                if (Entry.Value <= 0 || Entry.Key.IsNone())
                {
                        continue;
                }

                if (UItemData* ItemData = ResolveItemData(Entry.Key))
                {
                        Inventory->AddItem(ItemData, Entry.Value);
                }
        }

        if (USkillSystemComponent* SkillSystem = ResolveSkillSystem())
        {
                if (!Recipe.RequiredKnowledge.IsNone())
                {
                        SkillSystem->GrantKnowledge(Recipe.RequiredKnowledge, Recipe.KnowledgeOnTry);
                        if (bWasSuccessful && Recipe.KnowledgeOnSuccess > 0.f)
                        {
                                SkillSystem->GrantKnowledge(Recipe.RequiredKnowledge, Recipe.KnowledgeOnSuccess);
                        }
                }

                const ESkillDomain PrimaryDomain = SkillDefinitions::ResolveSkillDomainFromTag(Recipe.PrimarySkillTag);
                if (PrimaryDomain != ESkillDomain::None)
                {
                        const float XpAward = bWasSuccessful ? Recipe.SuccessXP : Recipe.FailXP;
                        if (XpAward > 0.f)
                        {
                                SkillSystem->GrantSkillXP(PrimaryDomain, XpAward);
                        }
                }
        }
}

float UCraftingSystemComponent::CalculateRecipeDuration(const UCraftingRecipe& Recipe) const
{
        const USkillSystemComponent* SkillSystem = ResolveSkillSystem();
        const ESkillDomain PrimaryDomain = SkillDefinitions::ResolveSkillDomainFromTag(Recipe.PrimarySkillTag);

        float PrimarySkillLevel = 0.f;
        if (SkillSystem && PrimaryDomain != ESkillDomain::None)
        {
                PrimarySkillLevel = SkillSystem->GetSkillValue(PrimaryDomain) / 10.f;
        }

        const float Alpha = FMath::Clamp(PrimarySkillLevel / 10.f, 0.f, 1.f);
        const float DurationScale = FMath::Lerp(1.2f, 0.6f, Alpha);
        return Recipe.BaseDuration * DurationScale;
}

bool UCraftingSystemComponent::EvaluateCraftSuccess(const UCraftingRecipe& Recipe) const
{
        const USkillSystemComponent* SkillSystem = ResolveSkillSystem();
        const ESkillDomain PrimaryDomain = SkillDefinitions::ResolveSkillDomainFromTag(Recipe.PrimarySkillTag);

        float PrimarySkillLevel = 0.f;
        if (SkillSystem && PrimaryDomain != ESkillDomain::None)
        {
                PrimarySkillLevel = SkillSystem->GetSkillValue(PrimaryDomain) / 10.f;
        }

        float SecondaryContribution = 0.f;
        if (SkillSystem)
        {
                for (const TPair<FName, int32>& Entry : Recipe.SecondarySkillWeights)
                {
                        const ESkillDomain Domain = SkillDefinitions::ResolveSkillDomainFromTag(Entry.Key);
                        if (Domain == ESkillDomain::None)
                        {
                                continue;
                        }

                        const float Level = SkillSystem->GetSkillValue(Domain) / 10.f;
                        SecondaryContribution += Level * Entry.Value;
                }
        }

        const float SkillScore = PrimarySkillLevel * 10.f + SecondaryContribution;
        const float SuccessChance = FMath::Clamp(0.15f + (SkillScore - Recipe.BaseDifficulty) * 0.01f, 0.05f, 0.98f);
        return FMath::FRand() <= SuccessChance;
}

UInventoryComponent* UCraftingSystemComponent::ResolveInventoryComponent() const
{
        if (CachedInventory.IsValid())
        {
                return CachedInventory.Get();
        }

        if (AActor* Owner = GetOwner())
        {
                        UInventoryComponent* Inventory = Owner->FindComponentByClass<UInventoryComponent>();
                        CachedInventory = Inventory;
                        return Inventory;
        }

        return nullptr;
}

USkillSystemComponent* UCraftingSystemComponent::ResolveSkillSystem() const
{
        if (CachedSkillSystem.IsValid())
        {
                return CachedSkillSystem.Get();
        }

        if (AActor* Owner = GetOwner())
        {
                        USkillSystemComponent* SkillSystem = Owner->FindComponentByClass<USkillSystemComponent>();
                        CachedSkillSystem = SkillSystem;
                        return SkillSystem;
        }

        return nullptr;
}

UItemData* UCraftingSystemComponent::ResolveItemData(const FName& ItemId) const
{
        if (ItemId.IsNone())
        {
                return nullptr;
        }

        if (UAssetManager* AssetManager = UAssetManager::GetIfValid())
        {
                const FPrimaryAssetId AssetId(TEXT("Item"), ItemId);
                const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(AssetId);
                return Cast<UItemData>(AssetPath.TryLoad());
        }

        return nullptr;
}

int32 UCraftingSystemComponent::CountItemInInventory(const FName& ItemId) const
{
        UInventoryComponent* Inventory = ResolveInventoryComponent();
        if (!Inventory)
        {
                return 0;
        }

        int32 TotalCount = 0;
        const TArray<FItemStack>& Slots = Inventory->GetSlots();
        for (const FItemStack& Slot : Slots)
        {
                if (!Slot.Item)
                {
                        continue;
                }

                const FPrimaryAssetId AssetId = Slot.Item->GetPrimaryAssetId();
                if (AssetId.PrimaryAssetType == TEXT("Item") && AssetId.PrimaryAssetName == ItemId)
                {
                        TotalCount += Slot.Quantity;
                }
        }

        return TotalCount;
}

void UCraftingSystemComponent::BroadcastCraftStarted(const UCraftingRecipe& Recipe, float Duration)
{
        const FText Name = Recipe.DisplayName.IsEmpty() ? FText::FromName(Recipe.GetFName()) : Recipe.DisplayName;
        OnCraftStarted.Broadcast(Name, Duration);
        UpdateProgressDelegates();
}

void UCraftingSystemComponent::BroadcastCraftFinished(UCraftingRecipe* Recipe, bool bWasSuccessful)
{
        OnCraftFinished.Broadcast(Recipe, bWasSuccessful);
}

void UCraftingSystemComponent::BroadcastCraftCancelled(const FName& Reason)
{
        OnCraftCancelled.Broadcast(Reason);
}

void UCraftingSystemComponent::UpdateProgressDelegates()
{
        if (!ActiveCraft.IsActive())
        {
                return;
        }

        const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
        const float Elapsed = FMath::Max(0.f, CurrentTime - ActiveCraft.StartTime);
        OnCraftProgress.Broadcast(Elapsed, ActiveCraft.Duration);
}

void UCraftingSystemComponent::OnRep_ActiveCraft()
{
        UpdateProgressDelegates();
}

void UCraftingSystemComponent::ClientCraftStarted_Implementation(UCraftingRecipe* Recipe, float Duration)
{
        if (Recipe)
        {
                BroadcastCraftStarted(*Recipe, Duration);
        }
}

void UCraftingSystemComponent::ClientCraftFinished_Implementation(UCraftingRecipe* Recipe, bool bWasSuccessful)
{
        BroadcastCraftFinished(Recipe, bWasSuccessful);
}

void UCraftingSystemComponent::ClientCraftCancelled_Implementation(FName Reason)
{
        BroadcastCraftCancelled(Reason);
}
