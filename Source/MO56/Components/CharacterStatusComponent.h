#pragma once

#include "Components/ActorComponent.h"
#include "CharacterStatusComponent.generated.h"

class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVitalsUpdated);

USTRUCT()
struct FCharacterStatusBarsSnapshot
{
        GENERATED_BODY()

        FCharacterStatusBarsSnapshot()
                : OutputCapacity(0.f)
                , EnduranceReserve(0.f)
                , Oxygenation(0.f)
                , Hydration(0.f)
                , HeatStrain(0.f)
                , Integrity(0.f)
                , HeartRate(0.f)
                , SystolicBP(0.f)
                , DiastolicBP(0.f)
                , SpO2(0.f)
                , CoreTemp(0.f)
                , BloodGlucose(0.f)
        {
        }

        FCharacterStatusBarsSnapshot(float InOutputCapacity, float InEnduranceReserve, float InOxygenation, float InHydration, float InHeatStrain, float InIntegrity, float InHeartRate, float InSystolicBP, float InDiastolicBP, float InSpO2, float InCoreTemp, float InBloodGlucose)
                : OutputCapacity(InOutputCapacity)
                , EnduranceReserve(InEnduranceReserve)
                , Oxygenation(InOxygenation)
                , Hydration(InHydration)
                , HeatStrain(InHeatStrain)
                , Integrity(InIntegrity)
                , HeartRate(InHeartRate)
                , SystolicBP(InSystolicBP)
                , DiastolicBP(InDiastolicBP)
                , SpO2(InSpO2)
                , CoreTemp(InCoreTemp)
                , BloodGlucose(InBloodGlucose)
        {
        }

        bool Equals(const FCharacterStatusBarsSnapshot& Other, float BarTolerance = 0.25f, float VitalTolerance = 0.5f) const
        {
                return FMath::IsNearlyEqual(OutputCapacity, Other.OutputCapacity, BarTolerance)
                        && FMath::IsNearlyEqual(EnduranceReserve, Other.EnduranceReserve, BarTolerance)
                        && FMath::IsNearlyEqual(Oxygenation, Other.Oxygenation, BarTolerance)
                        && FMath::IsNearlyEqual(Hydration, Other.Hydration, BarTolerance)
                        && FMath::IsNearlyEqual(HeatStrain, Other.HeatStrain, BarTolerance)
                        && FMath::IsNearlyEqual(Integrity, Other.Integrity, BarTolerance)
                        && FMath::IsNearlyEqual(HeartRate, Other.HeartRate, VitalTolerance)
                        && FMath::IsNearlyEqual(SystolicBP, Other.SystolicBP, VitalTolerance)
                        && FMath::IsNearlyEqual(DiastolicBP, Other.DiastolicBP, VitalTolerance)
                        && FMath::IsNearlyEqual(SpO2, Other.SpO2, VitalTolerance)
                        && FMath::IsNearlyEqual(CoreTemp, Other.CoreTemp, VitalTolerance)
                        && FMath::IsNearlyEqual(BloodGlucose, Other.BloodGlucose, VitalTolerance);
        }

        float OutputCapacity;
        float EnduranceReserve;
        float Oxygenation;
        float Hydration;
        float HeatStrain;
        float Integrity;
        float HeartRate;
        float SystolicBP;
        float DiastolicBP;
        float SpO2;
        float CoreTemp;
        float BloodGlucose;
};

/**
 * Component that simulates player vitals for HUD widgets and gameplay systems.
 *
 * Editor Implementation Guide:
 * 1. Add the component to your character Blueprint so its tick runs on both server and client instances.
 * 2. Use the defaults panel to tune initial vitals (Status|Energy/O2/Hydration/etc.) for the desired survival fantasy.
 * 3. Bind OnVitalsUpdated or read the BlueprintReadOnly fields in your CharacterStatusWidget to render status bars.
 * 4. Drive the BlueprintReadWrite inputs (Speed, AmbientTemp, LoadKg) from animation blueprints or environment managers.
 * 5. Call ConsumeFood/DrinkWater/ApplyInjury from gameplay events and persist values through your save subsystem.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MO56_API UCharacterStatusComponent : public UActorComponent
{
        GENERATED_BODY()

public:
        UCharacterStatusComponent();

        virtual void BeginPlay() override;
        virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
        // HUD Bars (0..100)
        UPROPERTY(BlueprintReadOnly, Category="Status|Bars")
        float OutputCapacityBar = 100.f;

        UPROPERTY(BlueprintReadOnly, Category="Status|Bars")
        float EnduranceReserveBar = 100.f;

        UPROPERTY(BlueprintReadOnly, Category="Status|Bars")
        float OxygenationBar = 100.f;

        UPROPERTY(BlueprintReadOnly, Category="Status|Bars")
        float HydrationBar = 100.f;

        UPROPERTY(BlueprintReadOnly, Category="Status|Bars")
        float HeatStrainBar = 0.f;

        UPROPERTY(BlueprintReadOnly, Category="Status|Bars")
        float IntegrityBar = 100.f;

        // Energy
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Energy")
        float BloodGlucose = 100.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Energy")
        float GlycogenMuscle = 1500.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Energy")
        float GlycogenLiver = 400.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Energy")
        float FatStore = 150000.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Energy")
        float GutEnergyFast = 0.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Energy")
        float GutEnergySlow = 0.f;

        // Cardio and oxygen
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|O2")
        float SpO2 = 98.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|O2")
        float HeartRate = 75.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|O2")
        float SystolicBP = 120.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|O2")
        float DiastolicBP = 80.f;

        // Hydration and heat
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Hydration")
        float TotalBodyWater = 42.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Heat")
        float CoreTemp = 37.0f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Heat")
        float SweatLossToday = 0.f;

        // Fatigue
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Fatigue")
        float AnaerobicDebt = 0.f;

        // Integrity
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Integrity")
        float WoundLevel = 0.f;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Status|Integrity")
        float BleedRate = 0.f;

        // Configurable limits and rates
        UPROPERTY(EditAnywhere, Category="Status|Config")
        float GlycogenMuscleMax = 2000.f;

        UPROPERTY(EditAnywhere, Category="Status|Config")
        float GlycogenLiverMax = 500.f;

        UPROPERTY(EditAnywhere, Category="Status|Config")
        float TBW_Normal = 42.f;

        UPROPERTY(EditAnywhere, Category="Status|Config")
        float MaxAerobicPower = 800.f;

        UPROPERTY(EditAnywhere, Category="Status|Config")
        float FatAccessRateMax = 300.f;

        UPROPERTY(EditAnywhere, Category="Status|Config")
        float AbsorbFastRate = 80.f;

        UPROPERTY(EditAnywhere, Category="Status|Config")
        float AbsorbSlowRate = 20.f;

        UPROPERTY(EditAnywhere, Category="Status|Config")
        float BarSmoothingAlpha = 0.1f;

        // Activity inputs
        UPROPERTY(BlueprintReadWrite, Category="Status|Input")
        float Speed = 0.f;

        UPROPERTY(BlueprintReadWrite, Category="Status|Input")
        float Slope = 0.f;

        UPROPERTY(BlueprintReadWrite, Category="Status|Input")
        float LoadKg = 0.f;

        UPROPERTY(BlueprintReadWrite, Category="Status|Input")
        float AmbientTemp = 21.f;

        UPROPERTY(BlueprintReadWrite, Category="Status|Input")
        float AltitudeMeters = 0.f;

        // Delegate for UI
        UPROPERTY(BlueprintAssignable, Category="Status")
        FOnVitalsUpdated OnVitalsUpdated;

public:
        UFUNCTION(BlueprintCallable, Category="Status|Intake")
        void ConsumeFood(float KcalFast, float KcalSlow);

        UFUNCTION(BlueprintCallable, Category="Status|Intake")
        void DrinkWater(float Liters);

        UFUNCTION(BlueprintCallable, Category="Status|Damage")
        void ApplyInjury(float NewWoundLevel, float NewBleedRate);

        UFUNCTION(BlueprintCallable, Category="Status|Env")
        void SetActivityInputs(float InSpeed, float InSlope, float InLoadKg);

        UFUNCTION(BlueprintCallable, Category="Status|Env")
        void SetEnvironment(float InAmbientTemp, float InAltitudeMeters);

protected:
        void UpdateMetabolicDemand(float DeltaTime);
        void UpdateStoresAndVitals(float DeltaTime);
        void ComputeBars();
        void SmoothBars();
        void BroadcastIfChanged();

private:
        float OutputCapacityBarTarget = 100.f;
        float EnduranceReserveBarTarget = 100.f;
        float OxygenationBarTarget = 100.f;
        float HydrationBarTarget = 100.f;
        float HeatStrainBarTarget = 0.f;
        float IntegrityBarTarget = 100.f;

        float CurrentPowerDemand = 0.f;
        float CurrentFatPower = 0.f;
        float CurrentCarbPower = 0.f;

        float BloodEnergyBufferKcal = 10.f;

        FCharacterStatusBarsSnapshot LastBroadcastSnapshot;
        bool bFirstUpdate = true;

        float ConsumeFromStore(float& StoreValue, float Amount);
        float CalculateBodyMassKg() const;
};

