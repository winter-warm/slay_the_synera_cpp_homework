#ifndef DEFAULTSKILL_H
#define DEFAULTSKILL_H

#include "entity/component/attackcomponent.h"

#include <QString>

void ReverseFuture(Character* caster, Character* target);
Skill skillFromName(const QString& name);

enum class SkillId {
    None,
    Fireball,
    Heal,
    Taunt,
    ReverseFuture
};

#endif // DEFAULTSKILL_H
