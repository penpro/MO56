#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MO56MenuGameMode.generated.h"

class AMO56MenuPlayerController;

UCLASS()
class UMO56MenuSettingsSave;

class MO56_API AMO56MenuGameMode : public AGameModeBase
{
        GENERATED_BODY()

public:
        AMO56MenuGameMode();

        virtual void BeginPlay() override;

        /** Returns the loaded menu settings object. */
        UMO56MenuSettingsSave* GetMenuSettings() const { return MenuSettings; }

protected:
        /** Cached save object that stores menu-specific configuration. */
        UPROPERTY()
        TObjectPtr<UMO56MenuSettingsSave> MenuSettings = nullptr;
};

