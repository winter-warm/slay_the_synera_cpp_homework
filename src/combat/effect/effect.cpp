#include "effect.h"

effect::effect(int id, std::string na, QJsonObject params):
    effectid(id),name(std::move(na)),params(std::move(params)) {}

bool effect::apply(Character* other){
    owner = other;
    if(applyFunc){
        return applyFunc(*this, other);
    }
    return true;
}

bool effect::remove(){
    if(removeFunc){
        return removeFunc(*this);
    }
    return true;
}

void effect::onBattleStart(BattleContext& context){
    if(onBattleStartFunc){onBattleStartFunc(*this, context);}
}

void effect::beforeAttack(AttackContext& context){
    if(beforeAttackFunc){beforeAttackFunc(*this, context);}
}

void effect::afterAttack(AttackContext& context){
    if(afterAttackFunc){afterAttackFunc(*this, context);}
}

void effect::beforeBeAttacked(BeAttackedContext& context){
    if(beforeBeAttackedFunc){beforeBeAttackedFunc(*this, context);}
}

void effect::afterBeAttacked(BeAttackedContext& context){
    if(afterBeAttackedFunc){afterBeAttackedFunc(*this, context);}
}

void effect::beforeHeal(HealContext& context){
    if(beforeHealFunc){beforeHealFunc(*this, context);}
}

void effect::afterSkill(SkillContext& context){
    if(afterSkillFunc){afterSkillFunc(*this, context);}
}

void effect::beforeAddBuff(AddBuffContext& context){
    if(beforeAddBuffFunc){beforeAddBuffFunc(*this, context);}
}

void effect::beforeDeath(DeathContext& context){
    if(beforeDeathFunc){beforeDeathFunc(*this, context);}
}

void effect::onTurnStart(TurnContext& context){
    if(onTurnStartFunc){onTurnStartFunc(*this, context);}
}
