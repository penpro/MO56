// Implementation: Use this widget as the list entry for the character skill menu. Create a
// blueprint subclass, bind the optional SkillIcon/SkillName/SkillRank/SkillInfoButton widgets,
// and place the widget in your list views. Clicking the info button will raise an event so the
// menu can show detailed history/progression text.
#pragma once

#include "Blueprint/UserWidget.h"
#include "UObject/SoftObjectPtr.h"
#include "SkillListEntryWidget.generated.h"

struct FSkillDomainProgress;
struct FSkillKnowledgeEntry;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

DECLARE_MULTICAST_DELEGATE_FiveParams(FSkillEntryInfoRequestedNative, const FText& /*Title*/, const FText& /*Rank*/, const FText& /*History*/, const FText& /*Tips*/, TSoftObjectPtr<UTexture2D> /*Icon*/);

/**
 * Reusable entry widget for displaying skills or knowledge entries.
 * Bind SkillIcon/SkillName/SkillRank/SkillInfoButton in the designer to surface the data in UI blueprints.
 *
 * Editor Implementation Guide:
 * 1. Create a Blueprint deriving from USkillListEntryWidget and bind the optional widgets to your designer controls.
 * 2. Style the entry background states (hovered/selected) using the widget tree or Slate brushes.
 * 3. Connect the info button's OnClicked event to a Blueprint handler that calls OnInfoRequested.Broadcast.
 * 4. Place the Blueprint class into UCharacterSkillMenu::SkillEntryWidgetClass so entries spawn automatically.
 * 5. Customize SetupFromSkill/SetupFromKnowledge overrides in Blueprint (if needed) to inject additional formatting.
 */
UCLASS()
class MO56_API USkillListEntryWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        USkillListEntryWidget(const FObjectInitializer& ObjectInitializer);

        /** Configures the widget using a skill domain entry. */
        void SetupFromSkill(const FSkillDomainProgress& Entry);

        /** Configures the widget using a knowledge entry. */
        void SetupFromKnowledge(const FSkillKnowledgeEntry& Entry);

        /** Event fired when the info button is clicked. */
        FSkillEntryInfoRequestedNative& OnInfoRequested() { return InfoRequested; }

protected:
        virtual void NativeConstruct() override;
        virtual void NativeDestruct() override;

private:
        void ApplyDisplayData(const FText& InName, const FText& InRank, const FText& InHistory, const FText& InTips, TSoftObjectPtr<UTexture2D> InIcon);
        void UpdateVisuals();

        UFUNCTION()
        void HandleInfoButtonClicked();

private:
        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UImage> SkillIcon;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> SkillName;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> SkillRank;

        UPROPERTY(meta = (BindWidgetOptional))
        TObjectPtr<UButton> SkillInfoButton;

        FText StoredName;
        FText StoredRank;
        FText StoredHistory;
        FText StoredTips;
        TSoftObjectPtr<UTexture2D> StoredIcon;

        FSkillEntryInfoRequestedNative InfoRequested;
};
