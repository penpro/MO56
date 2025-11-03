#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MO56PlayerController.h"
#include "MO56PossessMenuWidget.generated.h"

class UListView;

UCLASS(BlueprintType)
class MO56_API UMO56PossessablePawnListEntry : public UObject
{
        GENERATED_BODY()

public:
        UPROPERTY(BlueprintReadOnly, Category = "Possess")
        FMOPossessablePawnInfo Info;

        void SetInfo(const FMOPossessablePawnInfo& InInfo)
        {
                Info = InInfo;
        }
};

UCLASS()
class MO56_API UMO56PossessMenuWidget : public UUserWidget
{
        GENERATED_BODY()

public:
        void SetList(const TArray<FMOPossessablePawnInfo>& In);

        UFUNCTION(BlueprintCallable, Category = "Possess")
        void RequestPossessSelected();

protected:
        UPROPERTY(meta = (BindWidget))
        UListView* PawnList = nullptr;

        UPROPERTY()
        TArray<FMOPossessablePawnInfo> Current;

        UPROPERTY()
        TArray<TObjectPtr<UMO56PossessablePawnListEntry>> EntryObjects;
};
