#include "UI/CharacterStatusWidget.h"

#include "Components/CharacterStatusComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Math/UnrealMathUtility.h"
#include "Skills/SkillSystemComponent.h"
#include "Skills/SkillTypes.h"

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

void UCharacterStatusWidget::SetSkillSystemComponent(USkillSystemComponent* InSkillSystem)
{
        if (SkillSystemComponent.Get() == InSkillSystem)
        {
                return;
        }

        if (USkillSystemComponent* Current = SkillSystemComponent.Get())
        {
                Current->OnSkillStateChanged.RemoveAll(this);
        }

        SkillSystemComponent = InSkillSystem;

        if (USkillSystemComponent* NewSystem = SkillSystemComponent.Get())
        {
                NewSystem->OnSkillStateChanged.AddUObject(this, &UCharacterStatusWidget::RefreshSkillSummaries);
        }

        RefreshSkillSummaries();
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

        RefreshSkillSummaries();
}

void UCharacterStatusWidget::NativeDestruct()
{
        if (StatusComponent.IsValid())
        {
                StatusComponent->OnVitalsUpdated.RemoveDynamic(this, &UCharacterStatusWidget::RefreshFromStatus);
                StatusComponent.Reset();
        }

        if (USkillSystemComponent* Current = SkillSystemComponent.Get())
        {
                Current->OnSkillStateChanged.RemoveAll(this);
        }

        SkillSystemComponent.Reset();
        Super::NativeDestruct();
}

void UCharacterStatusWidget::RefreshSkillSummaries()
{
        if (!KnowledgeSummaryText && !SkillSummaryText)
        {
                return;
        }

        if (!SkillSystemComponent.IsValid())
        {
                if (KnowledgeSummaryText)
                {
                        KnowledgeSummaryText->SetText(NSLOCTEXT("CharacterStatus", "KnowledgeNone", "Knowledge: --"));
                }

                if (SkillSummaryText)
                {
                        SkillSummaryText->SetText(NSLOCTEXT("CharacterStatus", "SkillsNone", "Skills: --"));
                }

                return;
        }

        if (KnowledgeSummaryText)
        {
                TArray<FSkillKnowledgeEntry> KnowledgeEntries;
                SkillSystemComponent->GetKnowledgeEntries(KnowledgeEntries);

                KnowledgeEntries.Sort([](const FSkillKnowledgeEntry& A, const FSkillKnowledgeEntry& B)
                {
                        return A.Value > B.Value;
                });

                const int32 Count = FMath::Min(3, KnowledgeEntries.Num());
                FString KnowledgeLines;
                for (int32 Index = 0; Index < Count; ++Index)
                {
                        const FSkillKnowledgeEntry& Entry = KnowledgeEntries[Index];
                        KnowledgeLines += FString::Printf(TEXT("%s %d"), *Entry.DisplayName.ToString(), FMath::RoundToInt(Entry.Value));
                        if (Index + 1 < Count)
                        {
                                KnowledgeLines += TEXT("\n");
                        }
                }

                if (KnowledgeLines.IsEmpty())
                {
                        KnowledgeSummaryText->SetText(NSLOCTEXT("CharacterStatus", "KnowledgeEmpty", "Knowledge: 0"));
                }
                else
                {
                        KnowledgeSummaryText->SetText(FText::FromString(KnowledgeLines));
                }
        }

        if (SkillSummaryText)
        {
                TArray<FSkillDomainProgress> SkillEntries;
                SkillSystemComponent->GetSkillEntries(SkillEntries);

                SkillEntries.Sort([](const FSkillDomainProgress& A, const FSkillDomainProgress& B)
                {
                        return A.Value > B.Value;
                });

                const int32 Count = FMath::Min(3, SkillEntries.Num());
                FString SkillLines;
                for (int32 Index = 0; Index < Count; ++Index)
                {
                        const FSkillDomainProgress& Entry = SkillEntries[Index];
                        SkillLines += FString::Printf(TEXT("%s %d"), *Entry.DisplayName.ToString(), FMath::RoundToInt(Entry.Value));
                        if (Index + 1 < Count)
                        {
                                SkillLines += TEXT("\n");
                        }
                }

                if (SkillLines.IsEmpty())
                {
                        SkillSummaryText->SetText(NSLOCTEXT("CharacterStatus", "SkillsEmpty", "Skills: 0"));
                }
                else
                {
                        SkillSummaryText->SetText(FText::FromString(SkillLines));
                }
        }
}

