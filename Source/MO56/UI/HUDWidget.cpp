#include "UI/HUDWidget.h"

#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"
#include "Skills/SkillSystemComponent.h"
#include "UI/CharacterSkillMenu.h"
#include "UI/InspectionCountdownWidget.h"
#include "GameFramework/PlayerController.h"

void UHUDWidget::AddCharacterStatusWidget(UWidget* Widget)
{
        AddWidgetToContainer(CharacterStatusContainer.Get(), Widget, /*bClearChildren*/ false);
}

void UHUDWidget::SetCrosshairWidget(UWidget* Widget)
{
        AddWidgetToContainer(CrosshairContainer.Get(), Widget, /*bClearChildren*/ true);
}

void UHUDWidget::AddLeftInventoryWidget(UWidget* Widget)
{
        AddWidgetToContainer(LeftInventoryContainer.Get(), Widget, /*bClearChildren*/ false);
}

void UHUDWidget::AddRightInventoryWidget(UWidget* Widget)
{
        AddWidgetToContainer(RightInventoryContainer.Get(), Widget, /*bClearChildren*/ false);
}

void UHUDWidget::SetMiniMapWidget(UWidget* Widget)
{
        AddWidgetToContainer(MiniMapContainer.Get(), Widget, /*bClearChildren*/ true);
}

void UHUDWidget::SetGameMenuWidget(UWidget* Widget)
{
        AddWidgetToContainer(GameMenuContainer.Get(), Widget, /*bClearChildren*/ true);
}

void UHUDWidget::SetCharacterSkillWidget(UWidget* Widget)
{
        AddWidgetToContainer(CharacterSkillContainer.Get(), Widget, /*bClearChildren*/ true);
}

void UHUDWidget::AddWidgetToContainer(UPanelWidget* Container, UWidget* Widget, bool bClearChildren)
{
        if (!Container || !Widget)
        {
                return;
        }

        if (bClearChildren)
        {
                Container->ClearChildren();
        }

        Container->AddChild(Widget);
}


void UHUDWidget::ConfigureSkillMenu(TSubclassOf<UCharacterSkillMenu> InMenuClass, USkillSystemComponent* InSkillSystem)
{
        SkillMenuClass = InMenuClass;
        SetSkillSystemComponent(InSkillSystem);
}

UCharacterSkillMenu* UHUDWidget::EnsureSkillMenuInstance()
{
        if (!SkillMenuInstance && SkillMenuClass)
        {
                UCharacterSkillMenu* NewMenu = nullptr;

                if (APlayerController* OwningPlayer = GetOwningPlayer())
                {
                        NewMenu = CreateWidget<UCharacterSkillMenu>(OwningPlayer, SkillMenuClass);
                }
                else if (UWorld* World = GetWorld())
                {
                        NewMenu = CreateWidget<UCharacterSkillMenu>(World, SkillMenuClass);
                }

                if (NewMenu)
                {
                        SkillMenuInstance = NewMenu;
                        SetCharacterSkillWidget(SkillMenuInstance);
                        SkillMenuInstance->SetVisibility(ESlateVisibility::Collapsed);
                        SkillMenuInstance->InitWithSkills(SkillSystemRef.Get());
                }
        }

        return SkillMenuInstance.Get();
}

bool UHUDWidget::ToggleSkillMenu()
{
        UCharacterSkillMenu* Menu = EnsureSkillMenuInstance();
        if (!Menu)
        {
                return false;
        }

        const ESlateVisibility CurrentVisibility = Menu->GetVisibility();
        const bool bCollapsed = CurrentVisibility == ESlateVisibility::Collapsed || CurrentVisibility == ESlateVisibility::Hidden;

        if (bCollapsed)
        {
                Menu->RebuildFromSkills(SkillSystemRef.Get());
                Menu->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
                return true;
        }

        Menu->SetVisibility(ESlateVisibility::Collapsed);
        return false;
}

void UHUDWidget::SetSkillMenuVisibility(bool bVisible)
{
        UCharacterSkillMenu* Menu = EnsureSkillMenuInstance();
        if (!Menu)
        {
                return;
        }

        const ESlateVisibility Desired = bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;

        if (bVisible)
        {
                Menu->RebuildFromSkills(SkillSystemRef.Get());
        }

        if (Menu->GetVisibility() != Desired)
        {
                Menu->SetVisibility(Desired);
        }
}

bool UHUDWidget::IsSkillMenuVisible() const
{
        if (const UCharacterSkillMenu* Menu = SkillMenuInstance.Get())
        {
                const ESlateVisibility MenuVisibility = Menu->GetVisibility();
                return MenuVisibility != ESlateVisibility::Collapsed && MenuVisibility != ESlateVisibility::Hidden;
        }

        return false;
}

void UHUDWidget::ConfigureInspectionCountdown(TSubclassOf<UInspectionCountdownWidget> InCountdownClass)
{
        InspectionCountdownClass = InCountdownClass;
}

UInspectionCountdownWidget* UHUDWidget::EnsureCountdownOverlay()
{
        if (!InspectionCountdownInstance && InspectionCountdownClass)
        {
                UInspectionCountdownWidget* NewCountdown = nullptr;

                if (APlayerController* OwningPlayer = GetOwningPlayer())
                {
                        NewCountdown = CreateWidget<UInspectionCountdownWidget>(OwningPlayer, InspectionCountdownClass);
                }
                else if (UWorld* World = GetWorld())
                {
                        NewCountdown = CreateWidget<UInspectionCountdownWidget>(World, InspectionCountdownClass);
                }

                if (NewCountdown)
                {
                        InspectionCountdownInstance = NewCountdown;
                        InspectionCountdownInstance->AddToViewport(1000);
                        InspectionCountdownInstance->SetVisibility(ESlateVisibility::Hidden);
                        InspectionCountdownInstance->OnCancelRequested.AddDynamic(this, &UHUDWidget::HandleCountdownCancelRequested);
                }
        }

        return InspectionCountdownInstance.Get();
}

void UHUDWidget::ShowInspection(const FText& Description, float DurationSeconds)
{
        if (UInspectionCountdownWidget* Countdown = EnsureCountdownOverlay())
        {
                Countdown->ShowCountdown(Description, DurationSeconds);
                UpdateCountdownVisibility(true);
        }
}

void UHUDWidget::UpdateInspection(float ElapsedSeconds, float DurationSeconds)
{
        if (UInspectionCountdownWidget* Countdown = EnsureCountdownOverlay())
        {
                Countdown->UpdateCountdown(ElapsedSeconds, DurationSeconds);
        }
}

void UHUDWidget::HideInspection()
{
        HideCountdownInternal();
}

void UHUDWidget::SetSkillSystemComponent(USkillSystemComponent* InSkillSystem)
{
        USkillSystemComponent* Current = SkillSystemRef.Get();
        if (Current == InSkillSystem)
        {
                return;
        }

        if (Current)
        {
                UnbindSkillDelegates(Current);
        }

        SkillSystemRef = InSkillSystem;

        if (SkillMenuInstance)
        {
                SkillMenuInstance->InitWithSkills(SkillSystemRef.Get());
        }

        if (USkillSystemComponent* NewSystem = SkillSystemRef.Get())
        {
                BindSkillDelegates(NewSystem);
        }
}

void UHUDWidget::BindSkillDelegates(USkillSystemComponent* InSkillSystem)
{
        if (!InSkillSystem)
        {
                return;
        }

        InSkillSystem->OnInspectionStarted.AddDynamic(this, &UHUDWidget::HandleInspectionStarted);
        InSkillSystem->OnInspectionProgress.AddDynamic(this, &UHUDWidget::HandleInspectionProgress);
        InSkillSystem->OnInspectionCompleted.AddDynamic(this, &UHUDWidget::HandleInspectionCompleted);
        InSkillSystem->OnInspectionCancelled.AddDynamic(this, &UHUDWidget::HandleInspectionCancelled);
}

void UHUDWidget::UnbindSkillDelegates(USkillSystemComponent* InSkillSystem)
{
        if (!InSkillSystem)
        {
                return;
        }

        InSkillSystem->OnInspectionStarted.RemoveDynamic(this, &UHUDWidget::HandleInspectionStarted);
        InSkillSystem->OnInspectionProgress.RemoveDynamic(this, &UHUDWidget::HandleInspectionProgress);
        InSkillSystem->OnInspectionCompleted.RemoveDynamic(this, &UHUDWidget::HandleInspectionCompleted);
        InSkillSystem->OnInspectionCancelled.RemoveDynamic(this, &UHUDWidget::HandleInspectionCancelled);
}

void UHUDWidget::HandleInspectionStarted(const FText& Description, float Duration)
{
        ShowInspection(Description, Duration);
}

void UHUDWidget::HandleInspectionProgress(float Elapsed, float Duration)
{
        UpdateInspection(Elapsed, Duration);
}

void UHUDWidget::HandleInspectionCompleted()
{
        HideInspection();
}

void UHUDWidget::HandleInspectionCancelled(FName /*Reason*/)
{
        HideInspection();
}

void UHUDWidget::HandleCountdownCancelRequested()
{
        if (USkillSystemComponent* System = SkillSystemRef.Get())
        {
                System->CancelCurrentInspection(TEXT("InputInterrupt"));
        }

        HideInspection();
}

void UHUDWidget::UpdateCountdownVisibility(bool bVisible) const
{
        if (InspectionCountdownInstance)
        {
                InspectionCountdownInstance->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        }
}

void UHUDWidget::HideCountdownInternal()
{
        if (InspectionCountdownInstance)
        {
                InspectionCountdownInstance->HideCountdown();
        }

        UpdateCountdownVisibility(false);
}

void UHUDWidget::NativeDestruct()
{
        if (USkillSystemComponent* System = SkillSystemRef.Get())
        {
                UnbindSkillDelegates(System);
        }

        SkillSystemRef.Reset();

        if (InspectionCountdownInstance)
        {
                InspectionCountdownInstance->OnCancelRequested.RemoveDynamic(this, &UHUDWidget::HandleCountdownCancelRequested);
                InspectionCountdownInstance->RemoveFromParent();
                InspectionCountdownInstance = nullptr;
        }

        SkillMenuInstance = nullptr;

        Super::NativeDestruct();
}
