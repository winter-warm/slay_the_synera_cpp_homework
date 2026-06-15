#include "statscomponent.h"
#include "entity/character/character.h"


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
    if(mp>=maxmp){
        mp-=maxmp;
        return true;
    }
    return false;
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

    int dhp = context.damage * 100 / (defense + 100);


    if(dhp>shield){
        modifyHP(shield-dhp);
        modifySHIELF(-1*shield);
    }else{
        modifySHIELF(-1*dhp);
    }
    owner->getbuff().afterBeAttacked(context);
}

void StatsComponent::heal(int amount, Character* source){
    if(amount <= 0){
        return;
    }

    HealContext context{source, owner, amount, false};
    owner->getbuff().beforeHeal(context);
    if(context.cancelled || context.amount <= 0){
        return;
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
