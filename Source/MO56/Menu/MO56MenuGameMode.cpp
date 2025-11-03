#include "Menu/MO56MenuGameMode.h"

#include "Menu/MO56MenuPlayerController.h"
#include "Save/MO56MenuSettingsSave.h"

#include "Kismet/GameplayStatics.h"

AMO56MenuGameMode::AMO56MenuGameMode()
{
    PlayerControllerClass = AMO56MenuPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
    bStartPlayersAsSpectators = true;
    bUseSeamlessTravel = false;
}

void AMO56MenuGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!MenuSettings)
    {
        MenuSettings = Cast<UMO56MenuSettingsSave>(UGameplayStatics::LoadGameFromSlot(UMO56MenuSettingsSave::StaticSlotName,
            UMO56MenuSettingsSave::StaticUserIndex));
        if (!MenuSettings)
        {
            MenuSettings = Cast<UMO56MenuSettingsSave>(UGameplayStatics::CreateSaveGameObject(UMO56MenuSettingsSave::StaticClass()));
            UGameplayStatics::SaveGameToSlot(MenuSettings, UMO56MenuSettingsSave::StaticSlotName,
                                         UMO56MenuSettingsSave::StaticUserIndex);
        }
    }

    if (MenuSettings)
    {
        UE_LOG(LogTemp, Log, TEXT("Menu settings loaded: %s"), *GetNameSafe(MenuSettings));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load or create menu settings save"));
    }
}

