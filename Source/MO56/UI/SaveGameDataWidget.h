#pragma once

#include "Blueprint/UserWidget.h"
#include "Save/MO56SaveSubsystem.h"
#include "SaveGameDataWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveEntryLoadRequested, const FSaveGameSummary&, Summary);

/**
 * Widget responsible for presenting a single save slot summary.
 *
 * Editor Implementation Guide:
 * 1. Create a Blueprint derived from USaveGameDataWidget and design labels for slot name, timestamps, and playtime.
 * 2. Bind the optional text blocks and LoadButton via the BindWidget meta tag to connect C++ pointers.
 * 3. In your save menu Blueprint, spawn one widget per summary returned from UMO56SaveSubsystem::GetAvailableSaveSummaries.
 * 4. Bind OnLoadRequested in Blueprint to call into the subsystem's LoadGameBySlot or to prompt the player for confirmation.
 * 5. Localize date/time formatting inside FormatDateTime or override in Blueprint for custom locale handling.
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
        TObjectPtr<UTextBlock> SaveIdText;

        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UButton> LoadButton;

        UFUNCTION()
        void HandleLoadClicked();

private:
        void RefreshDisplay();
        FText FormatDateTime(const FDateTime& DateTime) const;

        FSaveGameSummary CachedSummary;
};
