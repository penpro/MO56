#include "UI/InspectionCountdownWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Input/Reply.h"

UInspectionCountdownWidget::UInspectionCountdownWidget(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{
        bIsFocusable = true;
}

void UInspectionCountdownWidget::NativeConstruct()
{
        Super::NativeConstruct();
        SetVisibility(ESlateVisibility::Hidden);
        CachedDuration = 0.f;
}

void UInspectionCountdownWidget::ShowCountdown(const FText& Description, float DurationSeconds)
{
        CachedDuration = FMath::Max(DurationSeconds, 0.f);

        if (DescriptionText)
        {
                DescriptionText->SetText(Description);
        }

        UpdateCountdown(0.f, CachedDuration);
        SetVisibility(ESlateVisibility::Visible);
}

void UInspectionCountdownWidget::UpdateCountdown(float ElapsedSeconds, float DurationSeconds)
{
        CachedDuration = FMath::Max(DurationSeconds, CachedDuration);
        const float SafeDuration = CachedDuration > 0.f ? CachedDuration : FMath::Max(DurationSeconds, 0.f);
        const float ClampedElapsed = SafeDuration > 0.f ? FMath::Clamp(ElapsedSeconds, 0.f, SafeDuration) : 0.f;
        const float Remaining = SafeDuration > 0.f ? SafeDuration - ClampedElapsed : 0.f;
        const float Fraction = SafeDuration > 0.f ? ClampedElapsed / SafeDuration : 1.f;

        if (CountdownProgress)
        {
                CountdownProgress->SetPercent(Fraction);
        }

        if (TimeRemainingText)
        {
                TimeRemainingText->SetText(FText::AsNumber(FMath::CeilToInt(Remaining)));
        }
}

void UInspectionCountdownWidget::HideCountdown()
{
        SetVisibility(ESlateVisibility::Hidden);
        CachedDuration = 0.f;

        if (CountdownProgress)
        {
                CountdownProgress->SetPercent(0.f);
        }

        if (TimeRemainingText)
        {
                TimeRemainingText->SetText(FText::GetEmpty());
        }

        if (DescriptionText)
        {
                DescriptionText->SetText(FText::GetEmpty());
        }
}

void UInspectionCountdownWidget::RequestCancelInspection()
{
        OnCancelRequested.Broadcast();
}

FReply UInspectionCountdownWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
        return HandleCancelFromInput();
}

FReply UInspectionCountdownWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
        return HandleCancelFromInput();
}

FReply UInspectionCountdownWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
        return HandleCancelFromInput();
}

FReply UInspectionCountdownWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InTouchEvent)
{
        return HandleCancelFromInput();
}

FReply UInspectionCountdownWidget::HandleCancelFromInput()
{
        RequestCancelInspection();
        return FReply::Handled();
}
