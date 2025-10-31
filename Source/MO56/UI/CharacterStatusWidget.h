#pragma once

#include "Blueprint/UserWidget.h"
#include "CharacterStatusWidget.generated.h"

class UCharacterStatusComponent;
class UProgressBar;
class UTextBlock;
class USkillSystemComponent;

UCLASS()
class MO56_API UCharacterStatusWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        UPROPERTY(meta = (BindWidget))
        TObjectPtr<UProgressBar> OutputCapacityPB;

        UPROPERTY(meta = (BindWidget))
        TObjectPtr<UProgressBar> EnduranceReservePB;

        UPROPERTY(meta = (BindWidget))
        TObjectPtr<UProgressBar> OxygenationPB;

        UPROPERTY(meta = (BindWidget))
        TObjectPtr<UProgressBar> HydrationPB;

        UPROPERTY(meta = (BindWidget))
        TObjectPtr<UProgressBar> HeatStrainPB;

        UPROPERTY(meta = (BindWidget))
        TObjectPtr<UProgressBar> IntegrityPB;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> HRText;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> BPText;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> SpO2Text;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> TempText;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> GlucoseText;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> KnowledgeSummaryText;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> SkillSummaryText;

public:
        UFUNCTION(BlueprintCallable)
        void SetStatusComponent(UCharacterStatusComponent* InComp);

        UFUNCTION(BlueprintCallable)
        void SetSkillSystemComponent(USkillSystemComponent* InSkillSystem);

        UFUNCTION()
        void RefreshFromStatus();

protected:
        virtual void NativeDestruct() override;

private:
        TWeakObjectPtr<UCharacterStatusComponent> StatusComponent;
        TWeakObjectPtr<USkillSystemComponent> SkillSystemComponent;

        UFUNCTION()
        void RefreshSkillSummaries();
};

