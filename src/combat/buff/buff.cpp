#include "buff.h"

#include "../effect/effect.h"

int buff::buff_id_count = 0;

void buff::apply(Character* owner){
    for(auto& ele:effects){
        ele->apply(owner);
    }
}

void buff::recordTrigger(BuffTrigger firedTrigger){
    if(maxtrigger > 0 && trigger == firedTrigger){
        ++triggerTime;
    }
}

void buff::onBattleStart(BattleContext& context){
    for(auto& ele:effects){
        ele->onBattleStart(context);
    }
    recordTrigger(BuffTrigger::OnBattleStart);
}

void buff::beforeAttack(AttackContext& context){
    for(auto& ele:effects){
        ele->beforeAttack(context);
    }
    recordTrigger(BuffTrigger::BeforeAttack);
}

void buff::afterAttack(AttackContext& context){
    for(auto& ele:effects){
        ele->afterAttack(context);
    }
    recordTrigger(BuffTrigger::AfterAttack);
}

void buff::beforeBeAttacked(BeAttackedContext& context){
    for(auto& ele:effects){
        ele->beforeBeAttacked(context);
    }
    recordTrigger(BuffTrigger::BeforeBeAttacked);
}

void buff::afterBeAttacked(BeAttackedContext& context){
    for(auto& ele:effects){
        ele->afterBeAttacked(context);
    }
    recordTrigger(BuffTrigger::AfterBeAttacked);
}

void buff::beforeHeal(HealContext& context){
    for(auto& ele:effects){
        ele->beforeHeal(context);
    }
    recordTrigger(BuffTrigger::BeforeHeal);
}

void buff::afterSkill(SkillContext& context){
    for(auto& ele:effects){
        ele->afterSkill(context);
    }
    recordTrigger(BuffTrigger::AfterSkill);
}

void buff::beforeAddBuff(AddBuffContext& context){
    for(auto& ele:effects){
        ele->beforeAddBuff(context);
    }
    recordTrigger(BuffTrigger::BeforeAddBuff);
}

void buff::beforeDeath(DeathContext& context){
    for(auto& ele:effects){
        ele->beforeDeath(context);
    }
    recordTrigger(BuffTrigger::BeforeDeath);
}

void buff::onTurnStart(TurnContext& context){
    for(auto& ele:effects){
        ele->onTurnStart(context);
    }
}

void buff::remove(Character* owner){
    (void)owner;
    for(auto& ele:effects){
        ele->remove();
    }
}

bool buff::update(TurnContext& context){
    if(duration >= 0.0f){
        remaining -= context.deltaTime;
    }

    if(!is_expired() && default_cooldown > 0.0f){
        cooldown -= context.deltaTime;
        if(cooldown <= 0.0f && !shouldRemove()){
            onTurnStart(context);
            cooldown = default_cooldown;
        }
    }

    return shouldRemove();
}
