#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MO56MenuSettingsSave.generated.h"

/** Persistent save slot that stores menu-only configuration such as volume and UI scaling. */
UCLASS(BlueprintType)
class MO56_API UMO56MenuSettingsSave : public USaveGame
{
        GENERATED_BODY()

public:
        UMO56MenuSettingsSave();

        /** Fixed slot name used when saving menu configuration. */
        static constexpr const TCHAR* StaticSlotName = TEXT("MO56_MenuSettings");

        /** Fixed user index for menu configuration saves. */
        static constexpr int32 StaticUserIndex = 0;

        /** Master volume slider value for the menu. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
        float MasterVolume = 1.0f;

        /** Last selected save profile identifier. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
        FGuid LastSelectedSaveId;

        /** Desired user interface scaling while in menus. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
        float UIScale = 1.0f;
};
