#ifndef DEFAULTSKILL_H
#define DEFAULTSKILL_H

#include "entity/component/attackcomponent.h"

#include <QString>

void ReverseFuture(Character* caster, Character* target);
void Savage(Character* caster,Character* target);
void PawnAdvance(Character* caster, Character* target);
void CastleWall(Character* caster, Character* target);
void KnightFork(Character* caster, Character* target);
void DiagonalSmite(Character* caster, Character* target);
void RoyalSweep(Character* caster, Character* target);
void KingCommand(Character* caster, Character* target);
Skill skillFromName(const QString& name);

enum class SkillId {
    None,
    Fireball,
    Heal,
    Taunt,
    ReverseFuture
};

#endif // DEFAULTSKILL_H
