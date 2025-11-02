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
        void HandleSaveEntryClicked(const FGuid& SaveId);
        UMO56SaveSubsystem* ResolveSubsystem() const;
        FText FormatEntryText(const FSaveIndexEntry& Entry) const;
        FText FormatDateTime(const FDateTime& DateTime) const;

        UFUNCTION()
        void HandleNewGameClicked();

        UFUNCTION()
        void HandleLoadClicked();

        UPROPERTY()
        TObjectPtr<UVerticalBox> RootBox = nullptr;

        UPROPERTY()
        TObjectPtr<UButton> NewGameButton = nullptr;

        UPROPERTY()
        TObjectPtr<UButton> LoadGameButton = nullptr;

        UPROPERTY()
        TObjectPtr<UScrollBox> SaveList = nullptr;

        TWeakObjectPtr<UMO56SaveSubsystem> CachedSubsystem;
};

