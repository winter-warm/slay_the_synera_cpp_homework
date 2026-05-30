#include "effect.h"
#include "entity/character/character.h"

namespace heal_effect{
effect::BeAttackedHandler reverseDamageToHeal();
effect::TurnHandler regenerationHeal();
}

effect::BeAttackedHandler heal_effect::reverseDamageToHeal()//1
{
    return [](effect& self, BeAttackedContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr){
            return;
        }

        const int defense = owner->getstats().getDEFENSE();
        const int healAmount = context.damage * 100 / (defense + 100);
        owner->getstats().modifyHP(healAmount);
        context.cancelled = true;
    };
}

effect::TurnHandler heal_effect::regenerationHeal()//4
{
    return [](effect& self, TurnContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.character != owner){
            return;
        }

        const int amount = self.getParams().value("amount").toInt(2);
        owner->getstats().modifyHP(amount);
    };
}
