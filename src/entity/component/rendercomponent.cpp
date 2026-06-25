#include "rendercomponent.h"

RenderComponent::RenderComponent(Character* owner):component(owner) {}

RenderComponent::RenderComponent(const RenderComponent& other,Character* owner)
    : component(other,owner)
    , visible(other.visible)
{}

void RenderComponent::setVisible(bool value)
{
    visible = value;
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

void RenderComponent::update()
{}

void RenderComponent::requestHitFlash()
{
    ++pendingHitFlashCount;
}

bool RenderComponent::takeHitFlash()
{
    if (pendingHitFlashCount <= 0) {
        return false;
    }
    --pendingHitFlashCount;
    return true;
}

void RenderComponent::requestAttackLunge(int targetId)
{
    if (targetId >= 0) {
        pendingAttackLungeTargets.push_back(targetId);
    }
}

std::vector<int> RenderComponent::takeAttackLungeTargets()
{
    std::vector<int> out;
    out.swap(pendingAttackLungeTargets);
    return out;
}

void RenderComponent::requestSkillBurst()
{
    ++pendingSkillBurstCount;
}

bool RenderComponent::takeSkillBurst()
{
    if (pendingSkillBurstCount <= 0) {
        return false;
    }
    --pendingSkillBurstCount;
    return true;
}
