#include "buffmanager.h"

BuffManager::BuffManager(Character* owner):owner(owner) {buffs.clear();}
BuffManager::~BuffManager() = default;

void BuffManager::append(std::unique_ptr<buff>& buf){
    if (!buf) {
        return;
    }

    AddBuffContext context{owner, buf.get(), false};
    beforeAddBuff(context);
    if (context.cancelled) {
        return;
    }

    buf->apply(owner);
    buffs.push_back(std::move(buf));
}

void BuffManager::onBattleStart(BattleContext& context){
    for(auto& ele:buffs){
        ele->onBattleStart(context);
    }
}

void BuffManager::beforeAttack(AttackContext& context){
    for(auto& ele:buffs){
        ele->beforeAttack(context);
    }
}

void BuffManager::afterAttack(AttackContext& context){
    for(auto& ele:buffs){
        ele->afterAttack(context);
    }
}

void BuffManager::beforeBeAttacked(BeAttackedContext& context){
    for(auto& ele:buffs){
        ele->beforeBeAttacked(context);
    }
}

void BuffManager::afterBeAttacked(BeAttackedContext& context){
    for(auto& ele:buffs){
        ele->afterBeAttacked(context);
    }
}

void BuffManager::beforeAddBuff(AddBuffContext& context){
    for(auto& ele:buffs){
        ele->beforeAddBuff(context);
    }
}

void BuffManager::beforeDeath(DeathContext& context){
    for(auto& ele:buffs){
        ele->beforeDeath(context);
    }
}

void BuffManager::onTurnStart(TurnContext& context){
    for(auto& ele:buffs){
        ele->onTurnStart(context);
    }
}

void BuffManager::update(TurnContext& context){
    for(auto it = buffs.begin(); it != buffs.end();){
        if((*it)->update(context)){
            (*it)->remove(owner);
            it = buffs.erase(it);
        }else{
            ++it;
        }
    }
}

void BuffManager::requestRemoveByTemplateId(int buffid){
    for(auto& ele:buffs){
        if(ele && ele->image_id == buffid){
            ele->requestRemove();
        }
    }
}

void BuffManager::clear(){
    buffs.clear();
}
