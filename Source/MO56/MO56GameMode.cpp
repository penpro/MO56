// Copyright Epic Games, Inc. All Rights Reserved.

#include "MO56GameMode.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Save/MO56SaveSubsystem.h"

AMO56GameMode::AMO56GameMode()
{
        bStartPlayersAsSpectators = true;
        bUseSeamlessTravel = true;
}

void AMO56GameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
        Super::HandleStartingNewPlayer_Implementation(NewPlayer);

        if (!NewPlayer)
        {
                return;
        }

        if (UGameInstance* GameInstance = GetGameInstance())
        {
                if (UMO56SaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>())
                {
                        SaveSubsystem->AssignAndPossessPersistentPawn(NewPlayer);
                }
        }
}

void AMO56GameMode::PostSeamlessTravel()
{
        Super::PostSeamlessTravel();

        if (UGameInstance* GameInstance = GetGameInstance())
        {
                if (UMO56SaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMO56SaveSubsystem>())
                {
                        if (UWorld* World = GetWorld())
                        {
                                for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
                                {
                                        if (APlayerController* PlayerController = It->Get())
                                        {
                                                SaveSubsystem->AssignAndPossessPersistentPawn(PlayerController);
                                        }
                                }
                        }
                }
        }
}
