#include "Crafting/BuildSiteActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"

#include "Crafting/CraftingRecipe.h"
#include "ItemData.h"

ABuildSiteActor::ABuildSiteActor()
{
        bReplicates = true;
        SetReplicateMovement(true);

        Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
        RootComponent = Root;

        BlueprintMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlueprintMesh"));
        BlueprintMesh->SetupAttachment(Root);
        BlueprintMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BlueprintMesh->SetGenerateOverlapEvents(false);
        BlueprintMesh->SetCanEverAffectNavigation(false);

        InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
        InteractionSphere->SetupAttachment(Root);
        InteractionSphere->SetSphereRadius(200.f);
        InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
        InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ABuildSiteActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);

        DOREPLIFETIME(ABuildSiteActor, ReplicatedMaterials);
        DOREPLIFETIME(ABuildSiteActor, bCompleted);
        DOREPLIFETIME(ABuildSiteActor, Recipe);
}

TMap<FName, int32> ABuildSiteActor::GetMaterialsRemaining() const
{
        return MaterialsRemaining;
}

void ABuildSiteActor::InitializeFromRecipe(UCraftingRecipe* InRecipe)
{
        if (!HasAuthority())
        {
                return;
        }

        Recipe = InRecipe;
        RebuildMaterialsFromRecipe();
        UpdateReplicatedMaterialsFromMap();
        NotifyProgressChanged();
}

bool ABuildSiteActor::ContributeMaterials(UInventoryComponent* Inventory)
{
        if (!Inventory || !Recipe || bCompleted || !HasAuthority())
        {
                return false;
        }

        bool bProgressed = false;
        for (auto& Pair : MaterialsRemaining)
        {
                if (Pair.Value <= 0)
                {
                        continue;
                }

                if (PullMaterialFromInventory(*Inventory, Pair.Key, Pair.Value))
                {
                        bProgressed = true;
                }

                Pair.Value = FMath::Max(0, Pair.Value);
        }

        bool bAllMet = true;
        for (const auto& Pair : MaterialsRemaining)
        {
                if (Pair.Value > 0)
                {
                        bAllMet = false;
                        break;
                }
        }

        if (bAllMet)
        {
                bCompleted = true;
                UpdateReplicatedMaterialsFromMap();
                NotifyProgressChanged();
                SpawnCompletedActor();
        }
        else
        {
                UpdateReplicatedMaterialsFromMap();
                NotifyProgressChanged();
        }

        return bProgressed;
}

void ABuildSiteActor::RebuildMaterialsFromRecipe()
{
        MaterialsRemaining.Empty();

        if (!Recipe)
        {
                return;
        }

        for (const TPair<FName, int32>& Pair : Recipe->BuildMaterialRequirements)
        {
                if (Pair.Value > 0)
                {
                        MaterialsRemaining.Add(Pair.Key, Pair.Value);
                }
        }
}

void ABuildSiteActor::SpawnCompletedActor()
{
        if (!Recipe || !Recipe->BuildableActorClass)
        {
                return;
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        GetWorld()->SpawnActor<AActor>(Recipe->BuildableActorClass, GetActorTransform(), Params);
        Destroy();
}

void ABuildSiteActor::NotifyProgressChanged()
{
        TArray<FBuildMaterialEntry> MaterialSnapshot;
        MaterialSnapshot.Reserve(MaterialsRemaining.Num());
        for (const TPair<FName, int32>& Pair : MaterialsRemaining)
        {
                MaterialSnapshot.Emplace(Pair.Key, Pair.Value);
        }

        OnBuildProgress.Broadcast(Recipe, MaterialSnapshot);

        if (BlueprintMesh && BlueprintFillParameter != NAME_None)
        {
                int32 TotalRequired = 0;
                int32 Remaining = 0;

                if (Recipe)
                {
                        for (const auto& Pair : Recipe->BuildMaterialRequirements)
                        {
                                TotalRequired += Pair.Value;
                                Remaining += MaterialsRemaining.FindRef(Pair.Key);
                        }
                }

                const float Progress = TotalRequired > 0 ? 1.f - (static_cast<float>(Remaining) / TotalRequired) : 1.f;
                BlueprintMesh->SetScalarParameterValueOnMaterials(BlueprintFillParameter, Progress);
        }

        if (bCompleted)
        {
                OnBuildCompleted.Broadcast(Recipe);
        }
}

bool ABuildSiteActor::PullMaterialFromInventory(UInventoryComponent& Inventory, const FName& ItemId, int32& CountRemaining)
{
        if (CountRemaining <= 0)
        {
                return false;
        }

        UAssetManager* AssetManager = UAssetManager::GetIfValid();
        if (!AssetManager)
        {
                return false;
        }

        const FPrimaryAssetId AssetId(TEXT("Item"), ItemId);
        const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(AssetId);
        UItemData* Item = Cast<UItemData>(AssetPath.TryLoad());
        if (!Item)
        {
                return false;
        }

        const int32 Removed = Inventory.RemoveItem(Item, CountRemaining);
        if (Removed > 0)
        {
                CountRemaining = FMath::Max(0, CountRemaining - Removed);
        }

        MaterialsRemaining.FindOrAdd(ItemId) = CountRemaining;
        return Removed > 0;
}

void ABuildSiteActor::OnRep_Materials()
{
        RebuildMaterialsMapFromReplicatedData();
        NotifyProgressChanged();
}

void ABuildSiteActor::OnRep_Completed()
{
        if (bCompleted)
        {
                RebuildMaterialsMapFromReplicatedData();
                NotifyProgressChanged();
        }
}

void ABuildSiteActor::UpdateReplicatedMaterialsFromMap()
{
        if (!HasAuthority())
        {
                return;
        }

        ReplicatedMaterials.Reset(MaterialsRemaining.Num());
        for (const TPair<FName, int32>& Pair : MaterialsRemaining)
        {
                ReplicatedMaterials.Emplace(Pair.Key, Pair.Value);
        }
}

void ABuildSiteActor::RebuildMaterialsMapFromReplicatedData()
{
        MaterialsRemaining.Empty(ReplicatedMaterials.Num());
        for (const FBuildMaterialEntry& Entry : ReplicatedMaterials)
        {
                MaterialsRemaining.Add(Entry.ItemId, Entry.Remaining);
        }
}
