#include "Components/MOPersistentPawnComponent.h"

UMOPersistentPawnComponent::UMOPersistentPawnComponent()
{
        PrimaryComponentTick.bCanEverTick = false;
        bAutoActivate = true;
}

void UMOPersistentPawnComponent::OnComponentCreated()
{
        Super::OnComponentCreated();

        if (!PawnId.IsValid())
        {
                PawnId = FGuid::NewGuid();
        }
}
