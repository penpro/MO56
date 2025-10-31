#include "UI/CharacterStatusWidget.h"

#include "Components/CharacterStatusComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Math/UnrealMathUtility.h"

void UCharacterStatusWidget::SetStatusComponent(UCharacterStatusComponent* InComp)
{
        if (StatusComponent.IsValid())
        {
                StatusComponent->OnVitalsUpdated.RemoveDynamic(this, &UCharacterStatusWidget::RefreshFromStatus);
        }

        StatusComponent = InComp;

        if (StatusComponent.IsValid())
        {
                StatusComponent->OnVitalsUpdated.AddDynamic(this, &UCharacterStatusWidget::RefreshFromStatus);
                RefreshFromStatus();
        }
}

void UCharacterStatusWidget::RefreshFromStatus()
{
        UCharacterStatusComponent* Comp = StatusComponent.Get();

        if (!Comp)
        {
                return;
        }

        const auto SetBar = [](UProgressBar* Bar, float Value)
        {
                if (Bar)
                {
                        Bar->SetPercent(FMath::Clamp(Value / 100.f, 0.f, 1.f));
                }
        };

        SetBar(OutputCapacityPB.Get(), Comp->OutputCapacityBar);
        SetBar(EnduranceReservePB.Get(), Comp->EnduranceReserveBar);
        SetBar(OxygenationPB.Get(), Comp->OxygenationBar);
        SetBar(HydrationPB.Get(), Comp->HydrationBar);
        SetBar(HeatStrainPB.Get(), Comp->HeatStrainBar);
        SetBar(IntegrityPB.Get(), Comp->IntegrityBar);

        if (HRText)
        {
                const int32 RoundedHR = FMath::RoundToInt(Comp->HeartRate);
                HRText->SetText(FText::Format(NSLOCTEXT("CharacterStatus", "HRFormat", "HR {0} bpm"), FText::AsNumber(RoundedHR)));
        }

        if (BPText)
        {
                const int32 Sys = FMath::RoundToInt(Comp->SystolicBP);
                const int32 Dia = FMath::RoundToInt(Comp->DiastolicBP);
                BPText->SetText(FText::Format(NSLOCTEXT("CharacterStatus", "BPFormat", "BP {0}/{1}"), FText::AsNumber(Sys), FText::AsNumber(Dia)));
        }

        if (SpO2Text)
        {
                const int32 RoundedSpO2 = FMath::RoundToInt(Comp->SpO2);
                SpO2Text->SetText(FText::Format(NSLOCTEXT("CharacterStatus", "SpO2Format", "SpO2 {0}%"), FText::AsNumber(RoundedSpO2)));
        }

        if (TempText)
        {
                const float RoundedTemp = FMath::RoundToFloat(Comp->CoreTemp * 10.f) / 10.f;
                TempText->SetText(FText::Format(NSLOCTEXT("CharacterStatus", "TempFormat", "Core {0}°C"), FText::AsNumber(RoundedTemp)));
        }

        if (GlucoseText)
        {
                const int32 RoundedGlucose = FMath::RoundToInt(Comp->BloodGlucose);
                GlucoseText->SetText(FText::Format(NSLOCTEXT("CharacterStatus", "GlucoseFormat", "Glucose {0} mg/dL"), FText::AsNumber(RoundedGlucose)));
        }
}

void UCharacterStatusWidget::NativeDestruct()
{
        if (StatusComponent.IsValid())
        {
                StatusComponent->OnVitalsUpdated.RemoveDynamic(this, &UCharacterStatusWidget::RefreshFromStatus);
                StatusComponent.Reset();
        }

        Super::NativeDestruct();
}

