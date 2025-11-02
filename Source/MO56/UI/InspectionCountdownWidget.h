#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectionCountdownWidget.generated.h"

class UProgressBar;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInspectionCancelRequested);

/**
 * Overlay widget that renders the active inspection countdown and handles cancellation input.
 *
 * Editor Implementation Guide:
 * 1. Create a Blueprint subclass and bind DescriptionText/TimeRemainingText/CountdownProgress to designer widgets.
 * 2. Style the overlay background and progress bar to match your HUD theme (e.g., translucent panel with iconography).
 * 3. Register the Blueprint class with UHUDWidget::ConfigureInspectionCountdown so characters spawn it during inspections.
 * 4. Use OnCancelRequested in Blueprint to trigger USkillSystemComponent::CancelCurrentInspection when players dismiss the UI.
 * 5. Override ShowCountdown/UpdateCountdown in Blueprint if you need additional animations or sound effects.
 */
UCLASS()
class MO56_API UInspectionCountdownWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        UInspectionCountdownWidget(const FObjectInitializer& ObjectInitializer);

        /** Displays the countdown with the supplied description and duration. */
        UFUNCTION(BlueprintCallable, Category = "Inspection")
        void ShowCountdown(const FText& Description, float DurationSeconds);

        /** Updates the countdown progress using elapsed and total duration values. */
        UFUNCTION(BlueprintCallable, Category = "Inspection")
        void UpdateCountdown(float ElapsedSeconds, float DurationSeconds);

        /** Hides the countdown overlay. */
        UFUNCTION(BlueprintCallable, Category = "Inspection")
        void HideCountdown();

        /** Requests that the inspection be cancelled. */
        UFUNCTION(BlueprintCallable, Category = "Inspection")
        void RequestCancelInspection();

        /** Fired whenever the widget requests that the inspection be cancelled. */
        UPROPERTY(BlueprintAssignable, Category = "Inspection")
        FOnInspectionCancelRequested OnCancelRequested;

protected:
        virtual void NativeConstruct() override;
        virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
        virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
        virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
        virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InTouchEvent) override;

private:
        FReply HandleCancelFromInput();

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> DescriptionText;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> TimeRemainingText;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UProgressBar> CountdownProgress;

        float CachedDuration = 0.f;
};
