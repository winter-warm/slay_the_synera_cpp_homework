#ifndef BUFFMANAGER_H
#define BUFFMANAGER_H
#include "buff.h"
#include <utility>
//buff已包含memory,vector

class Character;

class BuffManager
{
private:
    std::vector<std::unique_ptr<buff>> buffs;
    Character* owner = nullptr;
public:
    BuffManager(Character* owner);
    ~BuffManager();

    void append(std::unique_ptr<buff>& buf);
    void onBattleStart(BattleContext& context);
    void beforeAttack(AttackContext& context);
    void afterAttack(AttackContext& context);
    void beforeBeAttacked(BeAttackedContext& context);
    void afterBeAttacked(BeAttackedContext& context);
    void beforeHeal(HealContext& context);
    void afterSkill(SkillContext& context);
    void beforeAddBuff(AddBuffContext& context);
    void beforeDeath(DeathContext& context);
    void onTurnStart(TurnContext& context);
    void update(TurnContext& context);
    void requestRemoveByTemplateId(int buffid);
    void clear();
};

#endif // BUFFMANAGER_H
