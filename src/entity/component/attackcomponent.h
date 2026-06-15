#ifndef ATTACKCOMPONENT_H
#define ATTACKCOMPONENT_H
#include"component.h"
#include <functional>
#include <utility>
#include <vector>

struct Skill {
    using Handler = std::function<void(Character* caster, Character* target)>;

    Handler handler;
    bool is_buff = false;

    Skill() = default;
    Skill(Handler handler, bool isBuff = false)
        : handler(std::move(handler)), is_buff(isBuff) {}

    void execute(Character* caster, Character* target) const
    {
        if (handler) {
            handler(caster, target);
        }
    }
};

class AttackComponent:public component
{
private:
    std::vector<Skill> skill;
    float cooldown = 0.0f;
    float attackInterval = 1.0f;

public:
    AttackComponent(Character* owner,Skill skill);
    AttackComponent(Character* owner,std::vector<Skill> skills);
    AttackComponent(const AttackComponent& other,Character* owner);
    void update(float deltaTime,bool ismoving);
    void attack(Character* target);
    void useskill(Character* target);
    void activateAndRemoveBuffSkills();
    void modifyAttackIntervalPercent(int percent);
};

#endif // ATTACKCOMPONENT_H
