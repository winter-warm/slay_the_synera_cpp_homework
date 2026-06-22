#ifndef EFFECT_H
#define EFFECT_H

#include <functional>
#include <string>
#include <utility>

#include <QJsonObject>

#include "../context/combatcontext.h"

class Character;

class effect{
public:
    using ApplyHandler = std::function<bool(effect&, Character*)>;
    using RemoveHandler = std::function<bool(effect&)>;
    using BattleHandler = std::function<void(effect&, BattleContext&)>;
    using AttackHandler = std::function<void(effect&, AttackContext&)>;
    using BeAttackedHandler = std::function<void(effect&, BeAttackedContext&)>;
    using HealHandler = std::function<void(effect&, HealContext&)>;
    using SkillHandler = std::function<void(effect&, SkillContext&)>;
    using AddBuffHandler = std::function<void(effect&, AddBuffContext&)>;
    using DeathHandler = std::function<void(effect&, DeathContext&)>;
    using TurnHandler = std::function<void(effect&, TurnContext&)>;

    effect(int id, std::string na, QJsonObject params = {});
    ~effect() = default;

    int getEffectId() const{return effectid;}
    const std::string& getName() const{return name;}
    Character* getOwner() const{return owner;}
    const QJsonObject& getParams() const{return params;}

    bool apply(Character* other);
    bool remove();

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

    void setApply(ApplyHandler handler){applyFunc = std::move(handler);}
    void setRemove(RemoveHandler handler){removeFunc = std::move(handler);}
    void setOnBattleStart(BattleHandler handler){onBattleStartFunc = std::move(handler);}
    void setBeforeAttack(AttackHandler handler){beforeAttackFunc = std::move(handler);}
    void setAfterAttack(AttackHandler handler){afterAttackFunc = std::move(handler);}
    void setBeforeBeAttacked(BeAttackedHandler handler){beforeBeAttackedFunc = std::move(handler);}
    void setAfterBeAttacked(BeAttackedHandler handler){afterBeAttackedFunc = std::move(handler);}
    void setBeforeHeal(HealHandler handler){beforeHealFunc = std::move(handler);}
    void setAfterSkill(SkillHandler handler){afterSkillFunc = std::move(handler);}
    void setBeforeAddBuff(AddBuffHandler handler){beforeAddBuffFunc = std::move(handler);}
    void setBeforeDeath(DeathHandler handler){beforeDeathFunc = std::move(handler);}
    void setOnTurnStart(TurnHandler handler){onTurnStartFunc = std::move(handler);}

private:
    int effectid;
    std::string name;
    QJsonObject params;
    Character* owner = nullptr;
    ApplyHandler applyFunc;
    RemoveHandler removeFunc;

    BattleHandler onBattleStartFunc;//Handler 是 std::function 成员变量
    AttackHandler beforeAttackFunc;//function默认构造空函数std::function<void()> f;天生是空的
    AttackHandler afterAttackFunc;//所以不用初始化。而且现在等价于null
    BeAttackedHandler beforeBeAttackedFunc;
    BeAttackedHandler afterBeAttackedFunc;
    HealHandler beforeHealFunc;
    SkillHandler afterSkillFunc;
    AddBuffHandler beforeAddBuffFunc;
    DeathHandler beforeDeathFunc;
    TurnHandler onTurnStartFunc;
};

#endif // EFFECT_H
