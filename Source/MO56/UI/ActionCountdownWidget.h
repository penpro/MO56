#pragma once

#include "CoreMinimal.h"
#include "UI/InspectionCountdownWidget.h"
#include "ActionCountdownWidget.generated.h"

/**
 * Lightweight wrapper that reuses the inspection countdown widget for generic timed actions.
 *
 * Editor Implementation Guide:
 * 1. Create a Blueprint subclass if you need separate styling for non-inspection timers.
 * 2. Bind additional animations or audio cues in the Blueprint without altering the base countdown logic.
 * 3. Swap this class in UHUDWidget::ConfigureInspectionCountdown when using it for crafting/progress bars beyond inspections.
 * 4. Call ShowCountdown/UpdateCountdown/HideCountdown from gameplay Blueprints to drive the timer display.
 * 5. Use OnCancelRequested to abort the underlying action when players dismiss the countdown.
 */
UCLASS()
class MO56_API UActionCountdownWidget : public UInspectionCountdownWidget
{
        GENERATED_BODY()

public:
        UActionCountdownWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
