#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "BuildGhostActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

/**
 * Client-only placement hologram that previews buildable placement.
 *
 * Editor Implementation Guide:
 * 1. Create a Blueprint derivative to author ghost materials unique to each build recipe.
 * 2. Assign a translucent material instance with a scalar named by PlacementValidParameter (default bPlacementValid).
 * 3. Expose SetGhostMesh/Material from build mode logic to swap meshes per recipe selection.
 * 4. Call TestPlacementAtTransform from Blueprint before confirming placement to drive accept/deny messaging.
 * 5. Optionally bind UpdateGhostMaterialState in Blueprint to drive glow, audio, or Niagara indicators when invalid.
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
        bool TestPlacementAtTransform(
                const FTransform& TargetTransform,
                ECollisionChannel TraceChannel,
                FVector BoxHalfExtent,
                float SphereRadius,
                bool bUseBox);

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
