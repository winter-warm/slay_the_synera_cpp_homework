#include "effect.h"
#include "entity/character/character.h"
#include <algorithm>

namespace attribute_effect {
effect::BattleHandler statModifier();
effect::AttackHandler damageMultiplier();
effect::BeAttackedHandler damageReductionPercent();
effect::BeAttackedHandler damageImmunity();
effect::BeAttackedHandler lowHealthGuard();
effect::BeAttackedHandler attackUpAfterDamage();
effect::DeathHandler deathProtection();
}

effect::BattleHandler attribute_effect::statModifier()
{
    return [](effect& self, BattleContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.character != owner){
            return;
        }

        const QJsonObject& params = self.getParams();
        const QString stat = params.value("stat").toString();
        const int value = params.value("value").toInt();
        if(stat == "attack"){
            owner->getstats().modifyATTACL(value);
        }else if(stat == "attack_percent"){
            owner->getstats().modifyATTACL(owner->getstats().getATTACK() * value / 100);
        }else if(stat == "defense"){
            owner->getstats().modifyDEFESE(value);
        }else if(stat == "defense_percent"){
            owner->getstats().modifyDEFESE(owner->getstats().getDEFENSE() * value / 100);
        }else if(stat == "shield"){
            owner->getstats().modifySHIELF(value);
        }else if(stat == "shield_max_hp_percent"){
            owner->getstats().modifySHIELF(owner->getstats().getMAXHP() * value / 100);
        }else if(stat == "mp"){
            owner->getstats().modifyMP(value);
        }else if(stat == "max_mp"){
            owner->getstats().modifyMAXMP(value);
        }else if(stat == "attack_interval_percent"){
            owner->getattack().modifyAttackIntervalPercent(value);
        }
    };
}

effect::AttackHandler attribute_effect::damageMultiplier()
{
    return [](effect& self, AttackContext& context) {
        if(context.cancelled || context.attacker != self.getOwner()){
            return;
        }

        const int percent = self.getParams().value("percent").toInt(100);
        context.damage = context.damage * percent / 100;
    };
}

effect::BeAttackedHandler attribute_effect::damageReductionPercent()
{
    return [](effect& self, BeAttackedContext& context) {
        if(context.cancelled || context.target != self.getOwner()){
            return;
        }

        const int percent = self.getParams().value("percent").toInt(0);
        context.damage = context.damage * (100 - percent) / 100;
    };
}

effect::BeAttackedHandler attribute_effect::damageImmunity()
{
    return [](effect& self, BeAttackedContext& context) {
        if(context.target == self.getOwner()){
            context.cancelled = true;
        }
    };
}

effect::BeAttackedHandler attribute_effect::lowHealthGuard()
{
    return [](effect& self, BeAttackedContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.target != owner){
            return;
        }

        const int thresholdPercent = self.getParams().value("thresholdPercent").toInt(50);
        if(owner->getstats().getHP() * 100 > owner->getstats().getMAXHP() * thresholdPercent){
            return;
        }

        owner->getstats().modifySHIELF(self.getParams().value("shield").toInt(0));
        owner->getstats().modifyDEFESE(self.getParams().value("defense").toInt(0));
    };
}

effect::BeAttackedHandler attribute_effect::attackUpAfterDamage()
{
    return [](effect& self, BeAttackedContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.cancelled || context.target != owner){
            return;
        }

        const int percent = self.getParams().value("percent").toInt(5);
        const int delta = std::max(1, owner->getstats().getATTACK() * percent / 100);
        owner->getstats().modifyATTACL(delta);
    };
}

effect::DeathHandler attribute_effect::deathProtection()
{
    return [](effect& self, DeathContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.target != owner || context.cancelled){
            return;
        }

        const int healPercent = self.getParams().value("healPercent").toInt(50);
        const int targetHp = owner->getstats().getMAXHP() * healPercent / 100;
        const int delta = targetHp - owner->getstats().getHP();
        if(delta > 0){
            owner->getstats().heal(delta);
        }
        context.cancelled = true;
    };
}
