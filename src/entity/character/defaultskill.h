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
void KillingIntentSense(Character* caster, Character* target);
void EhEhOh(Character* caster, Character* target);
void Levitation(Character* caster, Character* target);
void SightInduction(Character* caster, Character* target);
void SimpleSpear(Character* caster, Character* target);
void Fireball(Character* caster, Character* target);
void Kotodama(Character* caster, Character* target);
void LiquidManipulation(Character* caster, Character* target);
void Gunshot(Character* caster, Character* target);
void DeathRewind(Character* caster, Character* target);
void Mimicry(Character* caster, Character* target);
void WitchKiller(Character* caster, Character* target);
Skill skillFromName(const QString& name, bool isBuff);

enum class SkillId {
    None,
    Fireball,
    Heal,
    Taunt,
    ReverseFuture
};

#endif // DEFAULTSKILL_H
