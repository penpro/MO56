#include "Crafting/BuildGhostActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"

ABuildGhostActor::ABuildGhostActor()
{
        PrimaryActorTick.bCanEverTick = false;
        SetActorEnableCollision(false);

        GhostMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GhostMesh"));
        RootComponent = GhostMeshComponent;

        GhostMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GhostMeshComponent->SetGenerateOverlapEvents(false);
        GhostMeshComponent->SetCanEverAffectNavigation(false);
        GhostMeshComponent->SetCastShadow(false);
}

void ABuildGhostActor::SetGhostMesh(UStaticMesh* Mesh)
{
        if (GhostMeshComponent)
        {
                GhostMeshComponent->SetStaticMesh(Mesh);
        }
}

void ABuildGhostActor::SetGhostMaterial(UMaterialInterface* Material)
{
        if (GhostMeshComponent)
        {
                const int32 MaterialCount = GhostMeshComponent->GetNumMaterials();
                for (int32 Index = 0; Index < MaterialCount; ++Index)
                {
                        GhostMeshComponent->SetMaterial(Index, Material);
                }
        }
}

void ABuildGhostActor::SetPlacementValid(bool bIsValid)
{
        bPlacementValid = bIsValid;
        UpdateGhostMaterialState();
}

bool ABuildGhostActor::TestPlacementAtTransform(const FTransform& TargetTransform, const FCollisionQueryParams& Params, const FCollisionShape& Shape)
{
        if (UWorld* World = GetWorld())
        {
                const FVector Start = TargetTransform.GetLocation();
                return !World->OverlapAnyTestByChannel(Start, TargetTransform.GetRotation(), ECC_WorldStatic, Shape, Params);
        }

        return false;
}

void ABuildGhostActor::OnConstruction(const FTransform& Transform)
{
        Super::OnConstruction(Transform);
        UpdateGhostMaterialState();
}

void ABuildGhostActor::UpdateGhostMaterialState()
{
        if (!GhostMeshComponent)
        {
                return;
        }

        if (PlacementValidParameter != NAME_None)
        {
                GhostMeshComponent->SetScalarParameterValueOnMaterials(PlacementValidParameter, bPlacementValid ? 1.f : 0.f);
        }
}
