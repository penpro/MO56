#include "Menu/MO56MenuGameMode.h"

#include "Menu/MO56MenuPlayerController.h"

AMO56MenuGameMode::AMO56MenuGameMode()
{
        PlayerControllerClass = AMO56MenuPlayerController::StaticClass();
        DefaultPawnClass = nullptr;
        bStartPlayersAsSpectators = true;
}

