#include "Components/MOPersistentPawnComponent.h"

#include "GameFramework/Actor.h"

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

        if (DisplayName.IsEmpty())
        {
                const AActor* OwnerActor = GetOwner();
                const FString OwnerName = OwnerActor ? OwnerActor->GetName() : TEXT("Unnamed");
                DisplayName = FText::FromString(OwnerName);
        }
}
