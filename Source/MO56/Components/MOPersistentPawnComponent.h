#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MO56VersionChecks.h"
#include "MOPersistentPawnComponent.generated.h"

UCLASS(ClassGroup=(Save), meta=(BlueprintSpawnableComponent))
class UMOPersistentPawnComponent : public UActorComponent
{
        GENERATED_BODY()

public:
        UMOPersistentPawnComponent();

        UPROPERTY(EditAnywhere, SaveGame)
        FGuid PawnId;

        UPROPERTY(EditAnywhere, SaveGame)
        bool bPlayerCandidate = true;

        virtual void OnComponentCreated() override;

        FGuid GetPawnId() const { return PawnId; }
};
