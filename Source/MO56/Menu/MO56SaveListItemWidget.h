#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MO56SaveListItemWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveChosen, FGuid, SaveId);

UCLASS(BlueprintType, Blueprintable)
class MO56_API UMO56SaveListItemWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        virtual void NativeConstruct() override;

        UFUNCTION(BlueprintCallable, Category = "Save")
        void InitFromData(const FString& Title, const FString& Meta, const FGuid& InId);

        UPROPERTY(meta = (BindWidget)) UButton* EntryButton = nullptr;

        UPROPERTY(meta = (BindWidget)) UTextBlock* TitleText = nullptr;

        UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* MetaText = nullptr;

        UPROPERTY(BlueprintReadOnly, Category = "Save")
        FGuid SaveId;

        UPROPERTY(BlueprintAssignable, Category = "Save")
        FOnSaveChosen OnSaveChosen;

protected:
        UFUNCTION()
        void HandleClicked();
};
