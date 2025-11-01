#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildGhostActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

/**
 * Client-only placement hologram that previews buildable placement.
 */
UCLASS()
class MO56_API ABuildGhostActor : public AActor
{
        GENERATED_BODY()

public:
        ABuildGhostActor();

        /** Assigns a static mesh to render for the hologram. */
        UFUNCTION(BlueprintCallable, Category = "Build")
        void SetGhostMesh(UStaticMesh* Mesh);

        /** Applies a material override for the ghost visual. */
        UFUNCTION(BlueprintCallable, Category = "Build")
        void SetGhostMaterial(UMaterialInterface* Material);

        /** Updates the placement validity flag which also controls the material parameter. */
        UFUNCTION(BlueprintCallable, Category = "Build")
        void SetPlacementValid(bool bIsValid);

        UFUNCTION(BlueprintPure, Category = "Build")
        bool IsPlacementValid() const { return bPlacementValid; }

        /** Performs a collision test against the provided transform. */
        UFUNCTION(BlueprintCallable, Category = "Build")
        bool TestPlacementAtTransform(const FTransform& TargetTransform, const FCollisionQueryParams& Params, const FCollisionShape& Shape);

protected:
        virtual void OnConstruction(const FTransform& Transform) override;

        void UpdateGhostMaterialState();

protected:
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        TObjectPtr<UStaticMeshComponent> GhostMeshComponent;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
        FName PlacementValidParameter = TEXT("bPlacementValid");

        bool bPlacementValid = true;
};
