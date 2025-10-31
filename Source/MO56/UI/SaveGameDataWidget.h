#pragma once

#include "Blueprint/UserWidget.h"
#include "Save/MO56SaveSubsystem.h"
#include "SaveGameDataWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveEntryLoadRequested, const FSaveGameSummary&, Summary);

/**
 * Widget responsible for presenting a single save slot summary.
 */
UCLASS()
class MO56_API USaveGameDataWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        virtual void NativeOnInitialized() override;

        /** Applies a summary to the widget for display. */
        void SetSummary(const FSaveGameSummary& Summary);

        /** Raised when the user presses the load button for this entry. */
        UPROPERTY(BlueprintAssignable, Category = "Save")
        FOnSaveEntryLoadRequested OnLoadRequested;

protected:
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> SlotNameText;

        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> TimePlayedText;

        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> LastSavedText;

        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> InitialSavedText;

        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> LastLevelText;

        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UTextBlock> InventoryCountText;

        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UButton> LoadButton;

        UFUNCTION()
        void HandleLoadClicked();

private:
        void RefreshDisplay();
        FText FormatDateTime(const FDateTime& DateTime) const;

        FSaveGameSummary CachedSummary;
};
