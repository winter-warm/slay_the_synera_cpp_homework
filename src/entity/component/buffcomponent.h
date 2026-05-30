#ifndef BUFFCOMPONENT_H
#define BUFFCOMPONENT_H

#include "component.h"
#include "../../combat/buff/buffmanager.h"
#include "../../combat/buff/buff.h"

class BuffComponent : public component
{
private:
    BuffManager buffmanager;
public:
    BuffComponent(Character* owner):buffmanager(owner),component(owner){}
    BuffComponent(const BuffComponent& other,Character* owner):component(other,owner),buffmanager(owner){}
    void addBuff(std::unique_ptr<buff> buf);
    void update(float deltaTime);
    void clear();
    void onBattleStart(BattleContext& context);
    void beforeAttack(AttackContext& context);
    void afterAttack(AttackContext& context);
    void beforeBeAttacked(BeAttackedContext& context);
    void afterBeAttacked(BeAttackedContext& context);
    void beforeAddBuff(AddBuffContext& context);
    void beforeDeath(DeathContext& context);
    void onTurnStart(TurnContext& context);
    void requestRemoveByTemplateId(int buffid);
};

#endif // BUFFCOMPONENT_H
