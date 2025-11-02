// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MO56GameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 *
 *  Editor Implementation Guide:
 *  1. Create a Blueprint subclass and assign it as the project default in Project Settings > Maps & Modes.
 *  2. Set DefaultPawnClass to your AMO56Character Blueprint and PlayerControllerClass to the MO56 controller Blueprint.
 *  3. Configure HUDClass/GameStateClass if you provide Blueprint overrides for UI or replicated state.
 *  4. Use GameMode Blueprint events (BeginPlay, OnPostLogin) to initialize crafting/skill systems or spawn UI managers.
 *  5. Combine with world settings (World Settings panel) to ensure server-travel maps load the same configuration.
 */
UCLASS(abstract)
class AMO56GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AMO56GameMode();
};



