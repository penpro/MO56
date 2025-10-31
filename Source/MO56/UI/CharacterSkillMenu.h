#pragma once

#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "CharacterSkillMenu.generated.h"

struct FSkillDomainProgress;
struct FSkillKnowledgeEntry;

class UPanelWidget;
class UTextBlock;
class USkillSystemComponent;

/**
 * Widget presenting knowledge and skill progression summaries.
 */
UCLASS()
class MO56_API UCharacterSkillMenu : public UUserWidget
{
        GENERATED_BODY()

public:
        UCharacterSkillMenu(const FObjectInitializer& ObjectInitializer);

        /** Assigns the skill system that drives the menu. */
        void SetSkillSystemComponent(USkillSystemComponent* InSkillSystem);

protected:
        virtual void NativeConstruct() override;
        virtual void NativeDestruct() override;

private:
        void RefreshSkillData();
        void RefreshInspectionStatus();
        void StartInspectionRefreshTimer();
        void StopInspectionRefreshTimer();

        UFUNCTION()
        void HandleSkillStateChanged();

        UFUNCTION()
        void HandleInspectionStateChanged();

        UFUNCTION()
        void HandleInspectionRefreshTimer();

        void RebuildKnowledgeList(const TArray<FSkillKnowledgeEntry>& KnowledgeEntries);
        void RebuildSkillList(const TArray<FSkillDomainProgress>& SkillEntries);

private:
        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> KnowledgeList;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> SkillList;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> InspectionStatusText;

        TWeakObjectPtr<USkillSystemComponent> SkillSystem;
        FTimerHandle InspectionRefreshHandle;
};
