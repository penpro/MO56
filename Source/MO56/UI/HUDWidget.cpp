#include "UI/HUDWidget.h"

#include "Components/PanelWidget.h"
#include "Components/Widget.h"

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

