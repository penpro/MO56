#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "WBP_RecipeBrowser.generated.h"

class SRecipeBrowserPanel;

UCLASS()
class MO56EDITOR_API UWBP_RecipeBrowser : public UEditorUtilityWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

    void RefreshRecipes();

private:
    TSharedPtr<SRecipeBrowserPanel> RecipeBrowserPanel;
};
