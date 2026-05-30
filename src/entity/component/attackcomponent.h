#ifndef ATTACKCOMPONENT_H
#define ATTACKCOMPONENT_H
#include"component.h"
#include <functional>
#include <vector>

using Skill = std::function<void(Character* caster, Character* target)>;

class AttackComponent:public component
{
private:
    std::vector<Skill> skill;
    int cooldown = 0;//1s一攻击

public:
    AttackComponent(Character* owner,Skill skill);
    AttackComponent(Character* owner,std::vector<Skill> skills);
    AttackComponent(const AttackComponent& other,Character* owner);
    void update(float deltaTime,bool ismoving);
    void attack(Character* target);
    void useskill(Character* target);
};

#endif // ATTACKCOMPONENT_H
