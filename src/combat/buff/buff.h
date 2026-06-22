#ifndef BUFF_H
#define BUFF_H

#include <memory>
#include <utility>
#include <vector>

#include <QString>

#include "../effect/effect.h"

class Character;
class BuffManager;

enum class BuffTrigger {
    None,
    OnBattleStart,
    BeforeAttack,
    AfterAttack,
    BeforeBeAttacked,
    AfterBeAttacked,
    BeforeHeal,
    AfterSkill,
    BeforeAddBuff,
    BeforeDeath
};

class buff
{
    friend class BuffManager;

private:
    QString name;
    std::vector<std::unique_ptr<effect>> effects;
    float duration;
    float remaining;
    float cooldown;
    float default_cooldown;
    short priority;
    int image_id;
    int buff_id;
    bool requestRemoved = false;
    int maxtrigger = -1;
    int triggerTime = 0;
    BuffTrigger trigger = BuffTrigger::None;
    static int buff_id_count;

public:
    explicit buff(int id, QString name, float duration, float default_cooldown,
                  int maxtrigger, BuffTrigger trigger,
                  std::vector<std::unique_ptr<effect>> effs, short pri = 0)
        : name(std::move(name)),
          effects(std::move(effs)),
          duration(duration),
          remaining(duration),
          cooldown(0.0f),
          default_cooldown(default_cooldown),
          priority(pri),
          image_id(id),
          buff_id(++buff_id_count),
          maxtrigger(maxtrigger),
          trigger(trigger)
    {}

    ~buff() = default;
    const QString& getName() const { return name; }
    bool is_expired() const { return duration >= 0.0f && remaining <= 0.0f; }
    void apply(Character* owner);
    void remove(Character* owner);
    bool update(TurnContext& context);
    void recordTrigger(BuffTrigger trigger);
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

    bool shouldRemove() const
    {
        return requestRemoved || is_expired() || (maxtrigger > 0 && triggerTime >= maxtrigger);
    }
    void requestRemove(){requestRemoved = true;}
};

#endif // BUFF_H
