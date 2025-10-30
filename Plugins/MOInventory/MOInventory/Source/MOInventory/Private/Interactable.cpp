#include "Interactable.h"
#include "GameFramework/Actor.h"

void IInteractable::Interact(AActor* Interactor)
{
}

FText IInteractable::GetInteractText() const
{
	return FText();
}
