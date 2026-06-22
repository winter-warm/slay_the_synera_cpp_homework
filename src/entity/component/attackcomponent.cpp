#include "attackcomponent.h"
#include "core/board.h"
#include "entity/character/character.h"
#include <algorithm>
#include <random>
#include <utility>

static std::mt19937& attackRng()
{
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}

AttackComponent::AttackComponent(Character* owner, Skill defaultSkill)
    : component(owner)
{
    skill.clear();
    skill.push_back(defaultSkill);
}

AttackComponent::AttackComponent(Character* owner, std::vector<Skill> skills)
    : component(owner), skill(std::move(skills))
{
    if(skill.empty()){
        skill.push_back(Skill{});
    }
}

AttackComponent::AttackComponent(const AttackComponent& other, Character* owner)
    : component(other, owner),
      skill(other.skill),
      cooldown(other.cooldown),
      attackInterval(other.attackInterval)
{}

void AttackComponent::update(float deltaTime, bool ismoving){
    cooldown -= deltaTime;
    if(ismoving || cooldown > 0.0f){
        return;
    }
    Character* target = owner->gettarget();
    if(target == nullptr || !target->isTargetable()){
        return;
    }
    attack(target);
    bool hasActiveSkill = false;
    for (const Skill& ele : skill) {
        if (!ele.is_buff && ele.handler) {
            hasActiveSkill = true;
            break;
        }
    }
    if(hasActiveSkill &&
       owner->getstats().getMAXMP() > 0 &&
       owner->getstats().getMP() >= owner->getstats().getMAXMP()){
        owner->getstats().modifyMP((-1) * owner->getstats().getMAXMP());
        owner->getrender().requestSkillBurst();
        useskill(target);
    }
}

void AttackComponent::attack(Character* target){
    if(target == nullptr || !target->isTargetable()){
        return;
    }
    Board* board = owner->getCurrentBoard();
    Hex selfHex;
    Hex targetHex;
    if(board &&
       (!board->posOf(owner, &selfHex) ||
        !board->posOf(target, &targetHex) ||
        board->dist(selfHex, targetHex) > owner->getstats().getRANGE())) {
        return;
    }

    AttackContext context{owner, target, owner->getstats().getATTACK(), false};
    owner->getbuff().beforeAttack(context);
    if(context.cancelled || context.target == nullptr){
        return;
    }
    if(context.critChancePercent > 0){
        std::uniform_int_distribution<int> distribution(1, 100);
        if(distribution(attackRng()) <= std::min(100, context.critChancePercent)){
            context.critical = true;
            context.damage += context.damage * std::max(0, context.critDamagePercent) / 100;
        }
    }
    owner->getrender().requestAttackLunge(context.target->id());
    context.target->getstats().beattacked(context.damage, owner);
    owner->getbuff().afterAttack(context);
    owner->getstats().modifyMP(1);
    cooldown = attackInterval;
}

void AttackComponent::useskill(Character* target){
    for(auto& ele : skill){
        ele.execute(owner, target);
    }
    SkillContext context{owner, target};
    owner->getbuff().afterSkill(context);
}

void AttackComponent::activateAndRemoveBuffSkills(){
    for(auto it = skill.begin(); it != skill.end();){
        if(it->is_buff){
            it->execute(owner, owner);
            it = skill.erase(it);
        }else{
            ++it;
        }
    }
}

void AttackComponent::modifyAttackIntervalPercent(int percent){
    attackInterval = attackInterval * percent / 100.0f;
    if(attackInterval < 0.1f){
        attackInterval = 0.1f;
    }
}
