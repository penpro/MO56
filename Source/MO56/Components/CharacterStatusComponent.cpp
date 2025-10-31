#include "Components/CharacterStatusComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector2D.h"

namespace CharacterStatus
{
        constexpr float JoulesPerKcal = 4184.f;
        constexpr float RestingPower = 100.f; // watts
        constexpr float BloodGlucoseBaseline = 90.f; // mg/dL baseline used for buffer calculations
        constexpr float BloodGlucoseMin = 60.f;
        constexpr float BloodGlucoseMax = 140.f;
        constexpr float CoreTempBaseline = 37.f;
        constexpr float CoreTempUpperSafe = 39.5f;
        constexpr float MaxSweatPerHour = 1.2f; // liters
        constexpr float MinutesPerHour = 60.f;
        constexpr float AnaerobicDebtRecoveryRate = 5.f; // units per minute
}

UCharacterStatusComponent::UCharacterStatusComponent()
{
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickInterval = 0.25f;
}

void UCharacterStatusComponent::BeginPlay()
{
        Super::BeginPlay();

        // Initialize the first broadcast snapshot with current values so that UI updates immediately.
        LastBroadcastSnapshot = FCharacterStatusBarsSnapshot(OutputCapacityBar, EnduranceReserveBar, OxygenationBar, HydrationBar, HeatStrainBar, IntegrityBar, HeartRate, SystolicBP, DiastolicBP, SpO2, CoreTemp, BloodGlucose);
        bFirstUpdate = true;
}

void UCharacterStatusComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        UpdateMetabolicDemand(DeltaTime);
        UpdateStoresAndVitals(DeltaTime);
        ComputeBars();
        SmoothBars();
        BroadcastIfChanged();
}

void UCharacterStatusComponent::ConsumeFood(float KcalFast, float KcalSlow)
{
        GutEnergyFast = FMath::Max(0.f, GutEnergyFast + KcalFast);
        GutEnergySlow = FMath::Max(0.f, GutEnergySlow + KcalSlow);

        BroadcastIfChanged();
}

void UCharacterStatusComponent::DrinkWater(float Liters)
{
        TotalBodyWater = FMath::Max(0.f, TotalBodyWater + Liters);
        SweatLossToday = FMath::Max(0.f, SweatLossToday - Liters);
        HydrationBarTarget = 100.f; // ensure smoothing pushes to hydrated state quickly
        BroadcastIfChanged();
}

void UCharacterStatusComponent::ApplyInjury(float NewWoundLevel, float NewBleedRate)
{
        WoundLevel = FMath::Clamp(NewWoundLevel, 0.f, 1.f);
        BleedRate = FMath::Max(0.f, NewBleedRate);
        BroadcastIfChanged();
}

void UCharacterStatusComponent::SetActivityInputs(float InSpeed, float InSlope, float InLoadKg)
{
        Speed = FMath::Max(0.f, InSpeed);
        Slope = FMath::Clamp(InSlope, -1.f, 1.f);
        LoadKg = FMath::Max(0.f, InLoadKg);
}

void UCharacterStatusComponent::SetEnvironment(float InAmbientTemp, float InAltitudeMeters)
{
        AmbientTemp = InAmbientTemp;
        AltitudeMeters = InAltitudeMeters;
}

void UCharacterStatusComponent::UpdateMetabolicDemand(float DeltaTime)
{
        const float BodyMassKg = CalculateBodyMassKg();
        const float SpeedMps = FMath::Max(0.f, Speed);
        const float Grade = FMath::Clamp(Slope, -1.f, 1.f);

        const float HorizontalResistance = BodyMassKg * SpeedMps * 3.5f;
        const float GravitationalComponent = BodyMassKg * 9.81f * SpeedMps * Grade;
        const float LoadPenalty = LoadKg * SpeedMps * 4.0f;
        const float AerobicFloor = CharacterStatus::RestingPower;

        const float RawPower = AerobicFloor + HorizontalResistance + GravitationalComponent + LoadPenalty;
        CurrentPowerDemand = FMath::Clamp(RawPower, 0.f, MaxAerobicPower * 1.5f);

        const float DemandAboveFat = FMath::Max(0.f, CurrentPowerDemand - FatAccessRateMax);
        CurrentFatPower = CurrentPowerDemand - DemandAboveFat;
        CurrentCarbPower = DemandAboveFat;

        if (CurrentPowerDemand < FatAccessRateMax)
        {
                const float CarbShare = FMath::GetMappedRangeValueClamped(FVector2D(0.f, FatAccessRateMax), FVector2D(0.15f, 0.6f), CurrentPowerDemand);
                CurrentCarbPower = CurrentPowerDemand * CarbShare;
                CurrentFatPower = CurrentPowerDemand - CurrentCarbPower;
        }
}

void UCharacterStatusComponent::UpdateStoresAndVitals(float DeltaTime)
{
        const float Minutes = DeltaTime / CharacterStatus::MinutesPerHour;
        const float Hours = DeltaTime / 3600.f;

        const float AbsorbFast = FMath::Min(GutEnergyFast, AbsorbFastRate * Minutes);
        GutEnergyFast -= AbsorbFast;

        const float AbsorbSlow = FMath::Min(GutEnergySlow, AbsorbSlowRate * Minutes);
        GutEnergySlow -= AbsorbSlow;

        BloodEnergyBufferKcal += AbsorbFast * 0.8f + AbsorbSlow * 0.5f;
        BloodEnergyBufferKcal = FMath::Clamp(BloodEnergyBufferKcal, 0.f, 40.f);

        BloodGlucose = FMath::Clamp(CharacterStatus::BloodGlucoseBaseline + BloodEnergyBufferKcal * 2.5f, CharacterStatus::BloodGlucoseMin, CharacterStatus::BloodGlucoseMax);

        const float CarbEnergyDemand = (CurrentCarbPower * DeltaTime) / CharacterStatus::JoulesPerKcal;
        float RemainingCarbDemand = CarbEnergyDemand;

        if (RemainingCarbDemand > 0.f)
        {
                const float BloodAvailable = FMath::Max(0.f, BloodEnergyBufferKcal);
                const float FromBlood = FMath::Min(BloodAvailable, RemainingCarbDemand);
                BloodEnergyBufferKcal -= FromBlood;
                RemainingCarbDemand -= FromBlood;
        }

        if (RemainingCarbDemand > 0.f)
        {
                RemainingCarbDemand -= ConsumeFromStore(GlycogenMuscle, RemainingCarbDemand);
        }

        if (RemainingCarbDemand > 0.f)
        {
                RemainingCarbDemand -= ConsumeFromStore(GlycogenLiver, RemainingCarbDemand);
        }

        if (RemainingCarbDemand > 0.f)
        {
                const float ConvertedFromFat = FMath::Min(FatStore, RemainingCarbDemand * 0.5f);
                FatStore -= ConvertedFromFat;
                RemainingCarbDemand -= ConvertedFromFat;
        }

        if (RemainingCarbDemand > KINDA_SMALL_NUMBER)
        {
                AnaerobicDebt += RemainingCarbDemand * 10.f;
                CurrentPowerDemand -= RemainingCarbDemand * CharacterStatus::JoulesPerKcal / DeltaTime;
                CurrentPowerDemand = FMath::Max(CurrentPowerDemand, 0.f);
        }

        AnaerobicDebt = FMath::Max(0.f, AnaerobicDebt - CharacterStatus::AnaerobicDebtRecoveryRate * Minutes);

        const float FatEnergyUsed = (CurrentFatPower * DeltaTime) / CharacterStatus::JoulesPerKcal;
        FatStore = FMath::Max(0.f, FatStore - FatEnergyUsed);

        const float AltitudeEffect = FMath::Clamp(AltitudeMeters / 1000.f * 1.5f, 0.f, 20.f);
        const float EffortEffect = FMath::Clamp(CurrentPowerDemand / MaxAerobicPower * 4.f, 0.f, 6.f);
        SpO2 = FMath::Clamp(98.f - AltitudeEffect - EffortEffect, 80.f, 100.f);

        HeartRate = FMath::Clamp(60.f + (CurrentPowerDemand / MaxAerobicPower) * 110.f + AnaerobicDebt * 0.1f, 50.f, 200.f);
        SystolicBP = FMath::Clamp(110.f + (CurrentPowerDemand / MaxAerobicPower) * 60.f + WoundLevel * 40.f, 90.f, 200.f);
        DiastolicBP = FMath::Clamp(70.f + (CurrentPowerDemand / MaxAerobicPower) * 25.f - WoundLevel * 20.f, 50.f, 140.f);

        const float HeatLoad = (CurrentPowerDemand / MaxAerobicPower) * 0.8f + FMath::Max(0.f, AmbientTemp - 21.f) * 0.03f;
        const float Cooling = FMath::Max(0.f, (TotalBodyWater / TBW_Normal) - 0.8f) * 0.5f;
        CoreTemp += (HeatLoad - Cooling) * Minutes * 0.2f;
        CoreTemp = FMath::Clamp(CoreTemp, CharacterStatus::CoreTempBaseline - 1.0f, CharacterStatus::CoreTempUpperSafe + 1.5f);

        const float SweatRate = FMath::Clamp(HeatLoad * 0.5f, 0.f, CharacterStatus::MaxSweatPerHour);
        const float SweatLoss = SweatRate * Hours;
        SweatLossToday += SweatLoss;
        TotalBodyWater = FMath::Max(0.f, TotalBodyWater - SweatLoss);

        if (BleedRate > 0.f)
        {
                const float BloodLoss = BleedRate * Hours;
                TotalBodyWater = FMath::Max(0.f, TotalBodyWater - BloodLoss);
        }

        BloodGlucose = FMath::Clamp(CharacterStatus::BloodGlucoseBaseline + BloodEnergyBufferKcal * 2.5f, CharacterStatus::BloodGlucoseMin, CharacterStatus::BloodGlucoseMax);
}

void UCharacterStatusComponent::ComputeBars()
{
        const float CapacityPercent = 100.f - (CurrentPowerDemand / FMath::Max(1.f, MaxAerobicPower)) * 100.f;
        OutputCapacityBarTarget = FMath::Clamp(CapacityPercent, 0.f, 100.f);

        const float GlycogenReserve = GlycogenMuscle + GlycogenLiver;
        const float GutReserve = GutEnergyFast + GutEnergySlow;
        const float TotalReserve = GlycogenMuscleMax + GlycogenLiverMax + 2000.f;
        const float EnduranceRatio = (GlycogenReserve + GutReserve) / FMath::Max(1.f, TotalReserve);
        EnduranceReserveBarTarget = FMath::Clamp(EnduranceRatio * 100.f, 0.f, 100.f);

        OxygenationBarTarget = FMath::Clamp(SpO2, 0.f, 100.f);

        const float HydrationRatio = TotalBodyWater / FMath::Max(0.1f, TBW_Normal);
        HydrationBarTarget = FMath::Clamp(HydrationRatio * 100.f, 0.f, 100.f);

        const float HeatStrainRatio = FMath::GetMappedRangeValueClamped(FVector2D(CharacterStatus::CoreTempBaseline - 0.5f, CharacterStatus::CoreTempUpperSafe), FVector2D(0.f, 100.f), CoreTemp);
        HeatStrainBarTarget = FMath::Clamp(HeatStrainRatio, 0.f, 100.f);

        const float IntegrityPercent = FMath::Clamp((1.f - WoundLevel) * 100.f - BleedRate * 2.f, 0.f, 100.f);
        IntegrityBarTarget = IntegrityPercent;
}

void UCharacterStatusComponent::SmoothBars()
{
        const auto Smooth = [Alpha = FMath::Clamp(BarSmoothingAlpha, 0.f, 1.f)](float Current, float Target)
        {
                return Current + Alpha * (Target - Current);
        };

        if (bFirstUpdate)
        {
                OutputCapacityBar = OutputCapacityBarTarget;
                EnduranceReserveBar = EnduranceReserveBarTarget;
                OxygenationBar = OxygenationBarTarget;
                HydrationBar = HydrationBarTarget;
                HeatStrainBar = HeatStrainBarTarget;
                IntegrityBar = IntegrityBarTarget;
                bFirstUpdate = false;
                return;
        }

        OutputCapacityBar = Smooth(OutputCapacityBar, OutputCapacityBarTarget);
        EnduranceReserveBar = Smooth(EnduranceReserveBar, EnduranceReserveBarTarget);
        OxygenationBar = Smooth(OxygenationBar, OxygenationBarTarget);
        HydrationBar = Smooth(HydrationBar, HydrationBarTarget);
        HeatStrainBar = Smooth(HeatStrainBar, HeatStrainBarTarget);
        IntegrityBar = Smooth(IntegrityBar, IntegrityBarTarget);
}

void UCharacterStatusComponent::BroadcastIfChanged()
{
        const FCharacterStatusBarsSnapshot CurrentSnapshot(OutputCapacityBar, EnduranceReserveBar, OxygenationBar, HydrationBar, HeatStrainBar, IntegrityBar, HeartRate, SystolicBP, DiastolicBP, SpO2, CoreTemp, BloodGlucose);

        if (!CurrentSnapshot.Equals(LastBroadcastSnapshot))
        {
                LastBroadcastSnapshot = CurrentSnapshot;
                OnVitalsUpdated.Broadcast();
        }
}

float UCharacterStatusComponent::ConsumeFromStore(float& StoreValue, float Amount)
{
        const float Consumed = FMath::Clamp(Amount, 0.f, StoreValue);
        StoreValue -= Consumed;
        return Consumed;
}

float UCharacterStatusComponent::CalculateBodyMassKg() const
{
        const AActor* OwnerActor = GetOwner();
        const ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor);

        float BaseMass = 80.f;

        if (const UCharacterMovementComponent* MoveComp = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr)
        {
                BaseMass = FMath::Max(40.f, MoveComp->Mass);
        }

        return BaseMass + LoadKg;
}

