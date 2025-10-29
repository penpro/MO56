#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class AActor;

UINTERFACE(BlueprintType)
class MOINVENTORY_API UInteractable : public UInterface
{
    GENERATED_BODY()
};

class MOINVENTORY_API IInteractable
{
    GENERATED_BODY()

public:
    // Called when something tries to interact
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interact")
    void Interact(AActor* Interactor);
    virtual void Interact_Implementation(AActor* Interactor) {}

    // The on-screen prompt, e.g. "Press E to interact"
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interact")
    FText GetInteractText() const;
    virtual FText GetInteractText_Implementation() const { return FText::GetEmpty(); }
};
