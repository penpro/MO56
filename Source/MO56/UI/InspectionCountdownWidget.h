#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectionCountdownWidget.generated.h"

class UProgressBar;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInspectionCancelRequested);

/** Overlay widget that renders the active inspection countdown and handles cancellation input. */
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
