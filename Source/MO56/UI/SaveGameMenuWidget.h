#pragma once

#include "Blueprint/UserWidget.h"
#include "SaveGameMenuWidget.generated.h"

class UScrollBox;
class USaveGameDataWidget;
class UMO56SaveSubsystem;
struct FSaveGameSummary;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveMenuLoadCompleted);

/**
 * In-game widget that lists available save slots and allows loading them.
 */
UCLASS()
class MO56_API USaveGameMenuWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        virtual void NativeConstruct() override;

        /** Assigns the save subsystem used for data retrieval and load requests. */
        void SetSaveSubsystem(UMO56SaveSubsystem* Subsystem);

        /** Refreshes the scroll list with the latest save summaries. */
        UFUNCTION(BlueprintCallable, Category = "Save")
        void RefreshSaveEntries();

        /** Event broadcast after a save load has been requested and completed successfully. */
        UPROPERTY(BlueprintAssignable, Category = "Save")
        FOnSaveMenuLoadCompleted OnSaveLoaded;

protected:
        /** Scroll box that holds generated save entry widgets. */
        UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
        TObjectPtr<UScrollBox> SaveList;

        /** Widget class used for each individual save entry. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save")
        TSubclassOf<USaveGameDataWidget> SaveEntryWidgetClass;

private:
        void RebuildEntries(const TArray<FSaveGameSummary>& Summaries);

        UFUNCTION()
        void HandleEntryLoadRequested(const FSaveGameSummary& Summary);

        UMO56SaveSubsystem* ResolveSubsystem() const;

        TWeakObjectPtr<UMO56SaveSubsystem> CachedSubsystem;

        UPROPERTY(Transient)
        TArray<TObjectPtr<USaveGameDataWidget>> EntryWidgets;
};
