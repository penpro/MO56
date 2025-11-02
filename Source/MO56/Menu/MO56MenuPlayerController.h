#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MO56MenuPlayerController.generated.h"

class UMO56MainMenuWidget;

UCLASS()
class MO56_API AMO56MenuPlayerController : public APlayerController
{
        GENERATED_BODY()

public:
        AMO56MenuPlayerController();

        virtual void BeginPlay() override;

protected:
        UPROPERTY(EditAnywhere, Category = "Menu")
        TSubclassOf<UMO56MainMenuWidget> MainMenuWidgetClass;

        UPROPERTY()
        TObjectPtr<UMO56MainMenuWidget> MainMenuWidgetInstance;
};

