#include "effect.h"

#include "entity/character/character.h"

namespace control_effect {
effect::ApplyHandler temporaryTeam(teams team);
effect::RemoveHandler clearTemporaryTeam();
effect::ApplyHandler untargetable();
effect::RemoveHandler clearUntargetable();
}

effect::ApplyHandler control_effect::temporaryTeam(teams team)
{
    return [team](effect&, Character* owner) {
        if(owner == nullptr){
            return false;
        }

        owner->getteam().setTemporaryTeam(team);
        return true;
    };
}

effect::RemoveHandler control_effect::clearTemporaryTeam()
{
    return [](effect& self) {
        Character* owner = self.getOwner();
        if(owner == nullptr){
            return false;
        }

        owner->getteam().clearTemporaryTeam();
        return true;
    };
}

effect::ApplyHandler control_effect::untargetable()
{
    return [](effect&, Character* owner) {
        if(owner == nullptr){
            return false;
        }

        owner->addUntargetableLock();
        return true;
    };
}

effect::RemoveHandler control_effect::clearUntargetable()
{
    return [](effect& self) {
        Character* owner = self.getOwner();
        if(owner == nullptr){
            return false;
        }

        owner->removeUntargetableLock();
        return true;
    };
}
