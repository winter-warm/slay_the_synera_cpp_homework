#include "buffcomponent.h"

void BuffComponent::addBuff(std::unique_ptr<buff> buf){
    buffmanager.append(buf);
}
void BuffComponent::update(float deltaTime){
    TurnContext context{owner, deltaTime};
    buffmanager.update(context);
}
void BuffComponent::clear(){
    buffmanager.clear();
}

void BuffComponent::onBattleStart(BattleContext& context){
    buffmanager.onBattleStart(context);
}

void BuffComponent::beforeAttack(AttackContext& context){
    buffmanager.beforeAttack(context);
}

void BuffComponent::afterAttack(AttackContext& context){
    buffmanager.afterAttack(context);
}

void BuffComponent::beforeBeAttacked(BeAttackedContext& context){
    buffmanager.beforeBeAttacked(context);
}

void BuffComponent::afterBeAttacked(BeAttackedContext& context){
    buffmanager.afterBeAttacked(context);
}

void BuffComponent::beforeHeal(HealContext& context){
    buffmanager.beforeHeal(context);
}

void BuffComponent::beforeAddBuff(AddBuffContext& context){
    buffmanager.beforeAddBuff(context);
}

void BuffComponent::beforeDeath(DeathContext& context){
    buffmanager.beforeDeath(context);
}

void BuffComponent::onTurnStart(TurnContext& context){
    buffmanager.onTurnStart(context);
}

void BuffComponent::requestRemoveByTemplateId(int buffid){
    buffmanager.requestRemoveByTemplateId(buffid);
}
