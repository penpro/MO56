#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/MO56SaveTypes.h"
#include "MO56GameMode.h"
#include "UObject/SoftObjectPtr.h"
#include "MO56MainMenuWidget.generated.h"

class UButton;
class UScrollBox;
class UMO56SaveListItemWidget;
class UMO56SaveSubsystem;
class UWorld;

UCLASS(BlueprintType, Blueprintable)
class MO56_API UMO56MainMenuWidget : public UUserWidget
{
        GENERATED_BODY()

public:
UMO56MainMenuWidget(const FObjectInitializer& ObjectInitializer);
virtual void NativeConstruct() override;

protected:
void RefreshSaveEntries();
void HandleSaveEntryClicked(const FGuid& SaveId) const;
UMO56SaveSubsystem* ResolveSubsystem() const;
static FText FormatEntryText(const FSaveIndexEntry& Entry);
static FText FormatDateTime(const FDateTime& DateTime);

UFUNCTION()
void HandleNewGameClicked();

UFUNCTION()
void HandleLoadClicked();

UFUNCTION()
void HandleSaveChosen(FGuid SaveId);
virtual void NativeDestruct() override;

UPROPERTY(meta=(BindWidget)) UButton* NewGameButton = nullptr;

UPROPERTY(meta=(BindWidget)) UButton* LoadGameButton = nullptr;

UPROPERTY(meta=(BindWidget)) UScrollBox* SaveList = nullptr;

UPROPERTY(EditDefaultsOnly, Category="Menu")
TSubclassOf<UMO56SaveListItemWidget> SaveItemWidgetClass;

UPROPERTY(EditAnywhere, Category="Menu", meta=(AllowedClasses="World"))
TSoftObjectPtr<UWorld> StartingMap;

UPROPERTY(EditAnywhere, Category="Menu")
FName StartingMapFallback = TEXT("TestLevel");

mutable TWeakObjectPtr<UMO56SaveSubsystem> CachedSubsystem;
};

