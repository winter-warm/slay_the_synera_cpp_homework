#ifndef COMBATCONTEXT_H
#define COMBATCONTEXT_H

class Character;
class buff;

struct BattleContext {
    Character* character = nullptr;
};

struct AttackContext {
    Character* attacker = nullptr;
    Character* target = nullptr;
    int damage = 0;
    bool cancelled = false;
    int critChancePercent = 0;
    int critDamagePercent = 100;
    bool critical = false;
};

struct BeAttackedContext {
    Character* attacker = nullptr;
    Character* target = nullptr;
    int damage = 0;
    bool cancelled = false;
    int dodgeChancePercent = 0;
    bool dodged = false;
};

struct HealContext {
    Character* source = nullptr;
    Character* target = nullptr;
    int amount = 0;
    bool cancelled = false;
    int healingDonePercent = 0;
    int healingTakenPercent = 0;
};

struct SkillContext {
    Character* caster = nullptr;
    Character* target = nullptr;
};

struct AddBuffContext {
    Character* target = nullptr;
    buff* incomingBuff = nullptr;
    bool cancelled = false;
};

struct DeathContext {
    Character* target = nullptr;
    bool cancelled = false;
};

struct TurnContext {
    Character* character = nullptr;
    float deltaTime = 0.0f;
};

#endif // COMBATCONTEXT_H
