#pragma once

#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPtr.h"
#include "CharacterSkillMenu.generated.h"

struct FSkillDomainProgress;
struct FSkillKnowledgeEntry;

class UPanelWidget;
class UTextBlock;
class USkillSystemComponent;
class USkillListEntryWidget;
class UImage;
class UTexture2D;

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
        void HandleSkillInfoRequested(const FText& Title, const FText& Rank, const FText& History, const FText& Tips, TSoftObjectPtr<UTexture2D> Icon);
        void ShowSkillInfo(const FText& Title, const FText& Rank, const FText& History, const FText& Tips, TSoftObjectPtr<UTexture2D> Icon);
        void HideSkillInfo();

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

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UPanelWidget> SkillInfoPanel;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> SkillInfoTitleText;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> SkillInfoRankText;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> SkillInfoHistoryText;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> SkillInfoTipsText;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UImage> SkillInfoIcon;

        UPROPERTY(EditAnywhere, Category = "Skills")
        TSubclassOf<USkillListEntryWidget> SkillEntryWidgetClass;

        TWeakObjectPtr<USkillSystemComponent> SkillSystem;
        FTimerHandle InspectionRefreshHandle;
};
