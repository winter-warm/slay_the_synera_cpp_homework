#ifndef RENDERCOMPONENT_H
#define RENDERCOMPONENT_H

#include "component.h"

class CharacterHUD;

class RenderComponent : public component
{
private:
    bool visible = true;
    CharacterHUD* hud = nullptr;
public:
    RenderComponent(Character* owner);
    RenderComponent(const RenderComponent& other,Character* owner);
    void bindHUD(CharacterHUD* characterHUD);
    CharacterHUD* getHUD() const;
    void setVisible(bool value);
    void show();
    void hide();
    bool inVisible() const;
    void update();
};

#endif // RENDERCOMPONENT_H
