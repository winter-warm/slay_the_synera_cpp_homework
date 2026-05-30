#include "rendercomponent.h"
#include "gui/HUD/characterhud.h"

RenderComponent::RenderComponent(Character* owner):component(owner) {}

RenderComponent::RenderComponent(const RenderComponent& other,Character* owner):component(other,owner){}

void RenderComponent::bindHUD(CharacterHUD* characterHUD)
{
    hud = characterHUD;
    if (hud) {
        hud->setVisible(visible);
    }
}

CharacterHUD* RenderComponent::getHUD() const
{
    return hud;
}

void RenderComponent::setVisible(bool value)
{
    visible = value;
    if (hud) {
        hud->setVisible(value);
    }
}

void RenderComponent::show()
{
    setVisible(true);
}

void RenderComponent::hide()
{
    setVisible(false);
}

bool RenderComponent::inVisible() const
{
    return visible;
}
