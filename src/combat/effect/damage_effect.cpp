#include "effect.h"
#include "entity/character/character.h"

namespace damage_effect{
effect::AttackHandler witchification();
effect::TurnHandler burningDamage();
}

effect::AttackHandler damage_effect::witchification()//2
{
    return [](effect& self, AttackContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.cancelled || context.target == nullptr){
            return;
        }

        const int amount = self.getParams().value("amount").toInt(2);
        owner->getstats().modifyHP((-1)*amount);
    };
}

effect::TurnHandler damage_effect::burningDamage()//3
{
    return [](effect& self, TurnContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.character != owner){
            return;
        }

        const int amount = self.getParams().value("amount").toInt(2);
        owner->getstats().modifyHP((-1)*amount);
    };
}
