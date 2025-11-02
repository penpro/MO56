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

UCLASS(BlueprintType, Blueprintable)
class MO56_API UMO56MainMenuWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        UMO56MainMenuWidget(const FObjectInitializer& ObjectInitializer);
        virtual void NativeConstruct() override;

protected:
        void RefreshSaveEntries();
        UFUNCTION()
        void HandleSaveEntryButtonPressed();
        UFUNCTION()
        void HandleSaveEntryButtonClicked();
        void HandleSaveEntryClicked(const FGuid& SaveId) const;
        UMO56SaveSubsystem* ResolveSubsystem() const;
        static FText FormatEntryText(const FSaveIndexEntry& Entry);
        static FText FormatDateTime(const FDateTime& DateTime);

        UFUNCTION()
        void HandleNewGameClicked();

        UFUNCTION()
        void HandleLoadClicked();
        virtual void NativeDestruct() override;

        UPROPERTY(meta=(BindWidget)) UButton* NewGameButton = nullptr;

        UPROPERTY(meta=(BindWidget)) UButton* LoadGameButton = nullptr;

        UPROPERTY(meta=(BindWidget)) UScrollBox* SaveList = nullptr;

        TMap<TWeakObjectPtr<UButton>, FGuid> SaveButtonIds;
        TWeakObjectPtr<UButton> PendingSaveButton;

        mutable TWeakObjectPtr<UMO56SaveSubsystem> CachedSubsystem;
};

