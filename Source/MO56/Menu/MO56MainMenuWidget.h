#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/MO56SaveTypes.h"
#include "MO56MainMenuWidget.generated.h"

class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UMO56SaveSubsystem;

UCLASS()
class MO56_API UMO56MainMenuWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        virtual void NativeConstruct() override;

protected:
        void BuildMenuLayout();
        void RefreshSaveEntries();
        UFUNCTION()
        void HandleSaveEntryButtonPressed();
        UFUNCTION()
        void HandleSaveEntryButtonClicked();
        void HandleSaveEntryClicked(const FGuid& SaveId) const;
        UMO56SaveSubsystem* ResolveSubsystem() const;
        static FText FormatEntryText(const FSaveIndexEntry& Entry);
        static FText FormatDateTime(const FDateTime& DateTime);
        virtual void OnWidgetRebuilt() override; // called after RebuildWidget

        UFUNCTION()
        void HandleNewGameClicked() const;

        UFUNCTION()
        void HandleLoadClicked();
        virtual void NativeDestruct() override;

        UPROPERTY() UVerticalBox* RootBox = nullptr;
        UPROPERTY() UButton*      NewGameButton = nullptr;
        UPROPERTY() UButton*      LoadGameButton = nullptr;
        UPROPERTY() UScrollBox*   SaveList = nullptr;

        TMap<TWeakObjectPtr<UButton>, FGuid> SaveButtonIds;
        TWeakObjectPtr<UButton> PendingSaveButton;

        mutable TWeakObjectPtr<UMO56SaveSubsystem> CachedSubsystem;
};

