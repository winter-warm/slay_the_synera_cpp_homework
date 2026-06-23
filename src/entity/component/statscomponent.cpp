#include "statscomponent.h"
#include "entity/character/character.h"

#include <algorithm>
#include <random>

static std::mt19937& statsRng()
{
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}

StatsComponent::StatsComponent(const StatsComponent& other,Character* owner):component(other,owner){
    hp = other.hp; maxhp = other.maxhp; defense = other.defense;
    mp = other.mp; maxmp = other.maxmp; attack = other.attack;
    shield=other.shield; range=other.range;
}

void StatsComponent::modifyMAXHP(int delta){
    maxhp+=delta;
    if(maxhp<=0){
        maxhp = 1;
    }
    if(hp>maxhp)hp = maxhp;
}

bool StatsComponent::modifyMP(int delta){
    mp+=delta;
    if(mp < 0){
        mp = 0;
    }
    return maxmp > 0 && mp >= maxmp;
}

void StatsComponent::modifyMAXMP(int delta){
    maxmp += delta;
    if(maxmp < 0){
        maxmp = 0;
    }
}

void StatsComponent::setHP(int value){
    hp = value;
    if(hp > maxhp){
        hp = maxhp;
    }
}

void StatsComponent::beattacked(int damage, Character* attacker){
    BeAttackedContext context{attacker, owner, damage, false};
    owner->getbuff().beforeBeAttacked(context);
    if(context.cancelled){
        return;
    }
    if(context.dodgeChancePercent > 0){
        std::uniform_int_distribution<int> distribution(1, 100);
        if(distribution(statsRng()) <= std::min(100, context.dodgeChancePercent)){
            context.dodged = true;
            context.cancelled = true;
            owner->getbuff().afterBeAttacked(context);
            return;
        }
    }

    int dhp = context.damage * 100 / (defense + 100);


    if(dhp>shield){
        modifyHP(shield-dhp);
        modifySHIELF(-1*shield);
    }else{
        modifySHIELF(-1*dhp);
    }
    owner->getrender().requestHitFlash();
    owner->getbuff().afterBeAttacked(context);
}

void StatsComponent::heal(int amount, Character* source){
    if(amount <= 0){
        return;
    }

    HealContext context{source, owner, amount, false};
    if(source && source != owner){
        source->getbuff().beforeHeal(context);
    }
    owner->getbuff().beforeHeal(context);
    if(context.cancelled || context.amount <= 0){
        return;
    }

    if(context.healingDonePercent != 0){
        context.amount += context.amount * context.healingDonePercent / 100;
    }
    if(context.healingTakenPercent != 0){
        context.amount += context.amount * context.healingTakenPercent / 100;
    }
    modifyHP(context.amount);
    if(hp > maxhp){
        hp = maxhp;
    }
}

bool StatsComponent::update(){
    if(hp <= 0){
        DeathContext deathContext{owner, false};
        owner->getbuff().beforeDeath(deathContext);
        if(deathContext.cancelled){
            return false;
        }
        return true;
    }
    return false;
}
