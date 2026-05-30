#include "attackcomponent.h"
//attackcomponent带入了character，间接带入了buffmanager，间接带入了effect，间接带入了context
//#include "combat/context/combatcontext.h"
#include "entity/character/character.h"//在cpp中再include容器类，因为component的h已经program once，所以不会循环引用

AttackComponent::AttackComponent(Character* owner,Skill defaultSkill):
    component(owner)
{
    skill.clear();
    skill.push_back(defaultSkill);
}

AttackComponent::AttackComponent(Character* owner,std::vector<Skill> skills):
    component(owner),skill(std::move(skills))
{
    if(skill.empty()){
        skill.push_back([](Character*, Character*){});
    }
}

AttackComponent::AttackComponent(const AttackComponent& other,Character* owner):
    component(other,owner),skill(other.skill){}//function类型不需要深拷贝

void AttackComponent::update(float deltaTime,bool ismoving){
    cooldown-=deltaTime;
    if(ismoving||cooldown>0){
        return;
    }
    Character* target = owner->gettarget();
    if(target == nullptr)return;
    attack(target);
    if(owner->getstats().getMP() >= owner->getstats().getMAXMP()){
        owner->getstats().modifyMP((-1)*owner->getstats().getMAXMP());
        useskill(target);
    }
}

void AttackComponent::attack(Character* target){
    AttackContext context{owner, target, owner->getstats().getATTACK(), false};
    owner->getbuff().beforeAttack(context);
    if(context.cancelled || context.target == nullptr){
        return;
    }
    context.target->getstats().beattacked(context.damage, owner);
    owner->getbuff().afterAttack(context);
    owner->getstats().modifyMP(1);
}

void AttackComponent::useskill(Character* target){
    for(auto& ele:skill){
        ele(owner,target);
    }
}
