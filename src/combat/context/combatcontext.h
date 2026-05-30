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
};

struct BeAttackedContext {
    Character* attacker = nullptr;
    Character* target = nullptr;
    int damage = 0;
    bool cancelled = false;
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
