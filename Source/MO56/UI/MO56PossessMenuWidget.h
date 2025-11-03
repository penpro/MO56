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

        UFUNCTION(BlueprintPure, Category = "Possess")
        FText GetDisplayText() const
        {
                if (!Info.DisplayName.IsEmpty())
                {
                        return Info.DisplayName;
                }

                return Info.PawnId.IsValid() ? FText::FromString(Info.PawnId.ToString()) : FText::GetEmpty();
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
        virtual void NativeConstruct() override;
        virtual void NativeDestruct() override;

        void HandlePawnItemClicked(UObject* Item);

protected:
        UPROPERTY(meta = (BindWidget))
        UListView* PawnList = nullptr;

        UPROPERTY()
        TArray<FMOPossessablePawnInfo> Current;

        UPROPERTY()
        TArray<TObjectPtr<UMO56PossessablePawnListEntry>> EntryObjects;
};
