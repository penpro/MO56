// Copyright Epic Games, Inc. All Rights Reserved.

#include "MO56GameMode.h"

#include "GameFramework/PlayerController.h"
#include "Save/MO56SaveSubsystem.h"

AMO56GameMode::AMO56GameMode()
{
        bStartPlayersAsSpectators = true;
}

void AMO56GameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
        Super::HandleStartingNewPlayer_Implementation(NewPlayer);

        if (!NewPlayer)
        {
                return;
        }

        if (UWorld* World = GetWorld())
        {
                if (UMO56SaveSubsystem* SaveSubsystem = World->GetSubsystem<UMO56SaveSubsystem>())
                {
                        SaveSubsystem->AssignAndPossessPersistentPawn(NewPlayer);
                }
        }
}
