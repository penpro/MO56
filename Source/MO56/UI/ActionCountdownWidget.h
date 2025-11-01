#pragma once

#include "CoreMinimal.h"
#include "UI/InspectionCountdownWidget.h"
#include "ActionCountdownWidget.generated.h"

/**
 * Lightweight wrapper that reuses the inspection countdown widget for generic timed actions.
 */
UCLASS()
class MO56_API UActionCountdownWidget : public UInspectionCountdownWidget
{
        GENERATED_BODY()

public:
        UActionCountdownWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
