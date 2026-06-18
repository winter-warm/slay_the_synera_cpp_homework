#ifndef RENDERCOMPONENT_H
#define RENDERCOMPONENT_H

#include "component.h"

#include <vector>

class CharacterHUD;

class RenderComponent : public component
{
private:
    bool visible = true;
    CharacterHUD* hud = nullptr;
    int pendingHitFlashCount = 0;
    int pendingSkillBurstCount = 0;
    std::vector<int> pendingAttackLungeTargets;
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
    void requestHitFlash();
    bool takeHitFlash();
    void requestAttackLunge(int targetId);
    std::vector<int> takeAttackLungeTargets();
    void requestSkillBurst();
    bool takeSkillBurst();
};

#endif // RENDERCOMPONENT_H
