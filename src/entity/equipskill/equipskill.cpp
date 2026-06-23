#include "equipskill.h"
#include "combat/buff/buff.h"
#include "combat/buff/bufffactory.h"
#include "combat/effect/effect.h"
#include "combat/equipment/equipment.h"
#include "entity/character/character.h"
#include "entity/component/movecomponent.h"

#include <QString>
#include <algorithm>
#include <cmath>
#include <random>
#include <utility>
#include <vector>

namespace equipskill {

static std::mt19937& equipmentRng() {
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}

static int attackIntervalPercentForSpeedBonus(int value) {
    return value <= -100 ? 10000 : 10000 / (100 + value);
}

static std::vector<Character*> boardCharacters(Character* owner, bool allies) {
    std::vector<Character*> out;
    if(!owner || !owner->getCurrentBoard()){
        return out;
    }
    for(Unit* unit : owner->getCurrentBoard()->units()){
        Character* character = dynamic_cast<Character*>(unit);
        if(!character || character->isdead()){
            continue;
        }
        const bool sameTeam = character->getteam().getteam() == owner->getteam().getteam();
        if(sameTeam == allies){
            out.push_back(character);
        }
    }
    return out;
}

static Character* lowestHpAlly(Character* owner) {
    Character* best = nullptr;
    for(Character* character : boardCharacters(owner, true)){
        if(!best || character->getstats().getHP() < best->getstats().getHP()){
            best = character;
        }
    }
    return best;
}

static Character* lowestMaxHpAlly(Character* owner) {
    Character* best = nullptr;
    for(Character* character : boardCharacters(owner, true)){
        if(!best || character->getstats().getMAXHP() < best->getstats().getMAXHP()){
            best = character;
        }
    }
    return best;
}

static void healPercentMaxHp(Character* source, Character* target, int percent) {
    if(!target || percent <= 0){
        return;
    }
    target->getstats().heal(std::max(1, target->getstats().getMAXHP() * percent / 100), source);
}

static std::unique_ptr<buff> temporaryAttackSpeedBuff(int bonusPercent, float duration) {
    const int applyPercent = attackIntervalPercentForSpeedBonus(bonusPercent);
    const int removePercent = applyPercent <= 0 ? 100 : 10000 / applyPercent;
    auto created = std::make_unique<effect>(401, "equipment_temporary_attack_speed");
    created->setApply([applyPercent](effect&, Character* owner) {
        if(owner){
            owner->getattack().modifyAttackIntervalPercent(applyPercent);
        }
        return true;
    });
    created->setRemove([removePercent](effect& self) {
        if(self.getOwner()){
            self.getOwner()->getattack().modifyAttackIntervalPercent(removePercent);
        }
        return true;
    });
    std::vector<std::unique_ptr<effect>> effects;
    effects.push_back(std::move(created));
    return std::make_unique<buff>(401, QStringLiteral("Equipment Temporary Attack Speed"), duration, 0.0f, -1, BuffTrigger::None, std::move(effects), 1);
}

static std::unique_ptr<buff> temporaryCritChanceBuff(int value, float duration) {
    auto created = std::make_unique<effect>(402, "equipment_temporary_crit_chance");
    created->setBeforeAttack([value](effect& self, AttackContext& context) {
        if(context.attacker == self.getOwner()){
            context.critChancePercent += value;
        }
    });
    std::vector<std::unique_ptr<effect>> effects;
    effects.push_back(std::move(created));
    return std::make_unique<buff>(402, QStringLiteral("Equipment Temporary Crit Chance"), duration, 0.0f, -1, BuffTrigger::None, std::move(effects), 1);
}

static std::unique_ptr<buff> temporaryDodgeBuff(int value, float duration) {
    auto created = std::make_unique<effect>(403, "equipment_temporary_dodge");
    created->setBeforeBeAttacked([value](effect& self, BeAttackedContext& context) {
        if(context.target == self.getOwner()){
            context.dodgeChancePercent += value;
        }
    });
    std::vector<std::unique_ptr<effect>> effects;
    effects.push_back(std::move(created));
    return std::make_unique<buff>(403, QStringLiteral("Equipment Temporary Dodge"), duration, 0.0f, -1, BuffTrigger::None, std::move(effects), 1);
}

static std::unique_ptr<buff> temporaryAttackPercentBuff(int value, float duration) {
    auto applied = std::make_shared<int>(0);
    auto created = std::make_unique<effect>(404, "equipment_temporary_attack_percent");
    created->setApply([value, applied](effect&, Character* owner) {
        if(owner){
            *applied = owner->getstats().getATTACK() * value / 100;
            owner->getstats().modifyATTACL(*applied);
        }
        return true;
    });
    created->setRemove([applied](effect& self) {
        if(self.getOwner()){
            self.getOwner()->getstats().modifyATTACL(-*applied);
        }
        return true;
    });
    std::vector<std::unique_ptr<effect>> effects;
    effects.push_back(std::move(created));
    return std::make_unique<buff>(404, QStringLiteral("Equipment Temporary Attack"), duration, 0.0f, -1, BuffTrigger::None, std::move(effects), 1);
}

static std::unique_ptr<buff> burningPercentBuff(Character* source, int percentPerSecond, float duration, int mpLossPerSecond = 0) {
    auto created = std::make_unique<effect>(405, "equipment_burning_percent");
    created->setOnTurnStart([source, percentPerSecond, mpLossPerSecond](effect& self, TurnContext& context) {
        Character* owner = self.getOwner();
        if(owner && context.character == owner){
            owner->getstats().beattacked(std::max(1, owner->getstats().getMAXHP() * percentPerSecond / 100), source);
            if(mpLossPerSecond > 0){
                owner->getstats().modifyMP(-mpLossPerSecond);
            }
        }
    });
    std::vector<std::unique_ptr<effect>> effects;
    effects.push_back(std::move(created));
    return std::make_unique<buff>(405, QStringLiteral("Equipment Burning"), duration, 1.0f, -1, BuffTrigger::None, std::move(effects), 1);
}

static std::unique_ptr<buff> delayedDamageBuff(Character* source, int totalDamage, float duration) {
    const int tickDamage = std::max(1, totalDamage / 3);
    auto created = std::make_unique<effect>(406, "equipment_delayed_damage");
    created->setOnTurnStart([source, tickDamage](effect& self, TurnContext& context) {
        Character* owner = self.getOwner();
        if(owner && context.character == owner){
            owner->getstats().beattacked(tickDamage, source);
        }
    });
    std::vector<std::unique_ptr<effect>> effects;
    effects.push_back(std::move(created));
    return std::make_unique<buff>(406, QStringLiteral("Equipment Delayed Damage"), duration, 1.0f, -1, BuffTrigger::None, std::move(effects), 1);
}

static void pushAttackerAway(Character* owner, Character* attacker) {
    if(!owner || !attacker || !owner->getCurrentBoard()){
        return;
    }
    Board* board = owner->getCurrentBoard();
    Hex ownerHex;
    Hex attackerHex;
    if(!board->posOf(owner, &ownerHex) || !board->posOf(attacker, &attackerHex)){
        return;
    }
    const Hex targetHex{attackerHex.x + (attackerHex.x - ownerHex.x),
                        attackerHex.y + (attackerHex.y - ownerHex.y),
                        attackerHex.z + (attackerHex.z - ownerHex.z)};
    if(board->passable(targetHex)){
        board->move(attacker, targetHex);
    }
}

static void teleportToRandomPassable(Character* owner) {
    if(!owner || !owner->getCurrentBoard()){
        return;
    }
    Board* board = owner->getCurrentBoard();
    std::vector<Hex> cells;
    for(const Hex& cell : board->cells()){
        if(board->passable(cell)){
            cells.push_back(cell);
        }
    }
    if(cells.empty()){
        return;
    }
    std::uniform_int_distribution<int> distribution(0, static_cast<int>(cells.size()) - 1);
    board->move(owner, cells[static_cast<size_t>(distribution(equipmentRng()))]);
}

static Character* nearestAlly(Character* owner) {
    Board* board = owner ? owner->getCurrentBoard() : nullptr;
    Hex ownerHex;
    if(!owner || !board || !board->posOf(owner, &ownerHex)){
        return nullptr;
    }

    Character* best = nullptr;
    int bestDistance = 0;
    for(Character* ally : boardCharacters(owner, true)){
        Hex allyHex;
        if(ally == owner || !board->posOf(ally, &allyHex)){
            continue;
        }
        const int distance = board->dist(ownerHex, allyHex);
        if(best == nullptr || distance < bestDistance){
            best = ally;
            bestDistance = distance;
        }
    }
    return best;
}

static std::unique_ptr<buff> moonFullAttackBuff() {
    auto used = std::make_shared<bool>(false);
    auto created = std::make_unique<effect>(407, "equipment_moon_full_attack");
    created->setBeforeAttack([used](effect& self, AttackContext& context) {
        Character* owner = self.getOwner();
        if(owner && context.attacker == owner && !*used){
            context.damage += std::max(1, owner->getstats().getATTACK() * 180 / 100);
            *used = true;
            owner->getbuff().requestRemoveByTemplateId(407);
        }
    });
    std::vector<std::unique_ptr<effect>> effects;
    effects.push_back(std::move(created));
    return std::make_unique<buff>(407, QStringLiteral("Moon Full Attack"), -1, 0.0f, -1, BuffTrigger::None, std::move(effects), 1);
}

static std::unique_ptr<buff> crownGuardianProtectBuff(Character* guardian) {
    const int guardianId = guardian ? guardian->id() : -1;
    auto created = std::make_unique<effect>(408, "equipment_crown_guardian_protect");
    created->setBeforeBeAttacked([guardianId](effect& self, BeAttackedContext& context) {
        Character* protectedAlly = self.getOwner();
        Board* board = protectedAlly ? protectedAlly->getCurrentBoard() : nullptr;
        Character* guardian = nullptr;
        if(board){
            for(Unit* unit : board->units()){
                Character* candidate = dynamic_cast<Character*>(unit);
                if(candidate && candidate->id() == guardianId){
                    guardian = candidate;
                    break;
                }
            }
        }
        if(protectedAlly == nullptr ||
           context.target != protectedAlly ||
           context.cancelled ||
           guardian == nullptr ||
           guardian == protectedAlly ||
           guardian->isdead()){
            return;
        }

        context.cancelled = true;
        guardian->getstats().beattacked(std::max(1, context.damage * 120 / 100), context.attacker);
    });
    std::vector<std::unique_ptr<effect>> effects;
    effects.push_back(std::move(created));
    return std::make_unique<buff>(408, QStringLiteral("Crown Guardian Protect"), -1, 0.0f, -1, BuffTrigger::None, std::move(effects), 1);
}

static std::unique_ptr<effect> equipmentModifierEffect(const EquipmentModifier& modifier) {
    auto created = std::make_unique<effect>(400, "equipment_modifier");
    switch (modifier.stat) {
    case EquipmentStat::AttackPercent:
    case EquipmentStat::AttackSpeedPercent:
    case EquipmentStat::AttackRangeFlat:
    case EquipmentStat::InitialMpFlat:
    case EquipmentStat::MaxMpFlat:
    case EquipmentStat::MaxHpPercent:
    case EquipmentStat::InitialShieldMaxHpPercent:
    case EquipmentStat::DefensePercent:
        created->setOnBattleStart([modifier](effect& self, BattleContext& context) {
            Character* owner = self.getOwner();
            if(owner == nullptr || context.character != owner){
                return;
            }

            StatsComponent& stats = owner->getstats();
            switch (modifier.stat) {
            case EquipmentStat::AttackPercent:
                stats.modifyATTACL(stats.getATTACK() * modifier.value / 100);
                break;
            case EquipmentStat::AttackSpeedPercent:
                owner->getattack().modifyAttackIntervalPercent(attackIntervalPercentForSpeedBonus(modifier.value));
                break;
            case EquipmentStat::AttackRangeFlat:
                stats.modifyRANGE(modifier.value);
                break;
            case EquipmentStat::InitialMpFlat:
                stats.modifyMP(modifier.value);
                break;
            case EquipmentStat::MaxMpFlat:
                stats.modifyMAXMP(modifier.value);
                break;
            case EquipmentStat::MaxHpPercent: {
                const int delta = stats.getMAXHP() * modifier.value / 100;
                stats.modifyMAXHP(delta);
                if(delta > 0){
                    stats.modifyHP(delta);
                }
                break;
            }
            case EquipmentStat::InitialShieldMaxHpPercent:
                stats.modifySHIELF(stats.getMAXHP() * modifier.value / 100);
                break;
            case EquipmentStat::DefensePercent:
                stats.modifyDEFESE(stats.getDEFENSE() * modifier.value / 100);
                break;
            default:
                break;
            }
        });
        break;
    case EquipmentStat::CritChancePercent:
        created->setBeforeAttack([modifier](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner()){
                context.critChancePercent += modifier.value;
            }
        });
        break;
    case EquipmentStat::CritDamagePercent:
        created->setBeforeAttack([modifier](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner()){
                context.critDamagePercent += modifier.value;
            }
        });
        break;
    case EquipmentStat::DodgeChancePercent:
        created->setBeforeBeAttacked([modifier](effect& self, BeAttackedContext& context) {
            if(context.target == self.getOwner()){
                context.dodgeChancePercent += modifier.value;
            }
        });
        break;
    case EquipmentStat::HealingDonePercent:
        created->setBeforeHeal([modifier](effect& self, HealContext& context) {
            if(context.source == self.getOwner()){
                context.healingDonePercent += modifier.value;
            }
        });
        break;
    case EquipmentStat::HealingTakenPercent:
        created->setBeforeHeal([modifier](effect& self, HealContext& context) {
            if(context.target == self.getOwner()){
                context.healingTakenPercent += modifier.value;
            }
        });
        break;
    }
    return created;
}

static std::unique_ptr<effect> equipmentSkillEffect(const std::string& skillName) {
    if(skillName.empty()){
        return nullptr;
    }

    auto created = std::make_unique<effect>(410, skillName);

    if(skillName == "equip_chiyan_zhan"){
        created->setAfterAttack([](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && context.target){
                std::uniform_int_distribution<int> distribution(1, 4);
                if(distribution(equipmentRng()) == 1){
                    const int percent = context.critical ? 300 : 90;
                    context.target->getstats().beattacked(std::max(1, self.getOwner()->getstats().getATTACK() * percent / 100), self.getOwner());
                }
            }
        });
    } else if(skillName == "equip_fenghen_lianzhan"){
        auto armed = std::make_shared<bool>(false);
        created->setAfterBeAttacked([armed](effect& self, BeAttackedContext& context) {
            if(context.target == self.getOwner() && context.dodged){
                *armed = true;
            }
        });
        created->setBeforeAttack([armed](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && *armed){
                context.damage += std::max(1, self.getOwner()->getstats().getATTACK() * 80 / 100);
                *armed = false;
            }
        });
    } else if(skillName == "equip_shuangshi_yazhi"){
        auto armed = std::make_shared<bool>(false);
        created->setAfterSkill([armed](effect& self, SkillContext& context) {
            if(context.caster == self.getOwner()){
                *armed = true;
            }
        });
        created->setAfterAttack([armed](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && context.target && *armed){
                context.target->getbuff().addBuff(temporaryAttackSpeedBuff(-80, 3.0f));
                *armed = false;
            }
        });
    } else if(skillName == "equip_yuehua_yishan"){
        auto used = std::make_shared<bool>(false);
        created->setAfterAttack([used](effect& self, AttackContext& context) {
            if(!*used && context.attacker == self.getOwner() && context.target && context.critical){
                context.target->getstats().beattacked(std::max(1, self.getOwner()->getstats().getATTACK() * 500 / 100), self.getOwner());
                *used = true;
            }
        });
    } else if(skillName == "equip_jiteng_fanji"){
        created->setAfterBeAttacked([](effect& self, BeAttackedContext& context) {
            if(context.target == self.getOwner() && context.attacker && !context.dodged){
                context.attacker->getstats().beattacked(std::max(1, context.damage * 30 / 100), self.getOwner());
            }
        });
    } else if(skillName == "equip_duanzui"){
        created->setBeforeAttack([](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && context.target &&
               context.target->getstats().getHP() * 100 < context.target->getstats().getMAXHP() * 40){
                context.critChancePercent += 100;
            }
        });
    } else if(skillName == "equip_liuxing_huoshi"){
        created->setAfterAttack([](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && context.target){
                context.target->getbuff().addBuff(burningPercentBuff(self.getOwner(), 3, 2.0f));
            }
        });
    } else if(skillName == "equip_fengying_lianshe"){
        auto count = std::make_shared<int>(0);
        created->setAfterAttack([count](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && context.target && ++(*count) % 3 == 0){
                context.target->getstats().beattacked(std::max(1, self.getOwner()->getstats().getATTACK() * 175 / 100), self.getOwner());
            }
        });
    } else if(skillName == "equip_bingleng_jian"){
        auto count = std::make_shared<int>(0);
        created->setAfterAttack([count](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && context.target && ++(*count) % 3 == 0){
                context.target->getbuff().addBuff(temporaryAttackSpeedBuff(-18, 3.0f));
            }
        });
    } else if(skillName == "equip_xinyue_lianxing"){
        created->setAfterAttack([](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && context.critical){
                self.getOwner()->getbuff().addBuff(temporaryAttackSpeedBuff(35, 2.0f));
            }
        });
    } else if(skillName == "equip_cuilin_bihu"){
        auto count = std::make_shared<int>(0);
        created->setAfterAttack([count](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && ++(*count) % 4 == 0){
                healPercentMaxHp(self.getOwner(), self.getOwner(), 20);
            }
        });
    } else if(skillName == "equip_wangguan_juji"){
        created->setBeforeAttack([](effect& self, AttackContext& context) {
            Board* board = self.getOwner() ? self.getOwner()->getCurrentBoard() : nullptr;
            Hex selfHex;
            Hex targetHex;
            if(context.attacker == self.getOwner() && context.target && board &&
               board->posOf(self.getOwner(), &selfHex) &&
               board->posOf(context.target, &targetHex) &&
               board->dist(selfHex, targetHex) == self.getOwner()->getstats().getRANGE()){
                context.critChancePercent += 35;
            }
        });
    } else if(skillName == "equip_honglian_gelie"){
        created->setAfterAttack([](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && context.target && context.critical){
                context.target->getbuff().addBuff(delayedDamageBuff(self.getOwner(), self.getOwner()->getstats().getATTACK() * 80 / 100, 3.0f));
            }
        });
    } else if(skillName == "equip_yanfan"){
        created->setAfterBeAttacked([](effect& self, BeAttackedContext& context) {
            if(context.target == self.getOwner() && context.attacker && context.dodged){
                context.attacker->getstats().beattacked(std::max(1, self.getOwner()->getstats().getATTACK() * 90 / 100), self.getOwner());
            }
        });
    } else if(skillName == "equip_bingya_tuxi"){
        auto count = std::make_shared<int>(0);
        created->setBeforeAttack([count](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner()){
                ++(*count);
                if(*count % 3 == 0){
                    context.critChancePercent += 100;
                }
            }
        });
        created->setAfterAttack([count](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && context.target && *count % 3 == 0){
                context.target->getbuff().addBuff(temporaryAttackSpeedBuff(-20, 3.0f));
            }
        });
    } else if(skillName == "equip_yeqiangwei"){
        created->setBeforeAttack([](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && context.target &&
               context.target->getstats().getHP() * 100 < context.target->getstats().getMAXHP() * 50){
                context.critDamagePercent += 100;
            }
        });
    } else if(skillName == "equip_xueji_jiqu"){
        created->setAfterAttack([](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner()){
                self.getOwner()->getstats().heal(std::max(1, self.getOwner()->getstats().getATTACK() * 30 / 100), self.getOwner());
            }
        });
    } else if(skillName == "equip_wangshi_beici"){
        auto firstAttack = std::make_shared<bool>(true);
        created->setOnBattleStart([](effect& self, BattleContext& context) {
            if(context.character == self.getOwner()){
                self.getOwner()->getbuff().addBuff(bufffactory::create(203, -1));
                self.getOwner()->getmove().setTargetSelector(TargetSelectorKind::LowestHpEnemy);
            }
        });
        created->setBeforeAttack([firstAttack](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && *firstAttack){
                context.damage *= 5;
                self.getOwner()->getbuff().requestRemoveByTemplateId(203);
                *firstAttack = false;
            }
        });
    } else if(skillName == "equip_chixing_baofa"){
        auto armed = std::make_shared<bool>(false);
        created->setAfterSkill([armed](effect& self, SkillContext& context) {
            if(context.caster == self.getOwner()){
                *armed = true;
            }
        });
        created->setBeforeAttack([armed](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner() && *armed){
                context.critChancePercent += 100;
                *armed = false;
            }
        });
    } else if(skillName == "equip_fengyu_huixiang"){
        created->setAfterSkill([](effect& self, SkillContext& context) {
            if(context.caster == self.getOwner()){
                self.getOwner()->getbuff().addBuff(temporaryAttackSpeedBuff(52, 4.0f));
            }
        });
    } else if(skillName == "equip_jiguang_hanliu"){
        created->setAfterSkill([](effect& self, SkillContext& context) {
            if(context.caster == self.getOwner() && context.target){
                context.target->getbuff().addBuff(temporaryAttackSpeedBuff(-30, 4.0f));
            }
        });
    } else if(skillName == "equip_senling_biyou"){
        created->setAfterSkill([](effect& self, SkillContext& context) {
            if(context.caster == self.getOwner()){
                healPercentMaxHp(self.getOwner(), lowestHpAlly(self.getOwner()), 20);
            }
        });
    } else if(skillName == "equip_wangguan_chiling"){
        auto used = std::make_shared<bool>(false);
        created->setAfterSkill([used](effect& self, SkillContext& context) {
            if(context.caster == self.getOwner() && !*used){
                for(Character* ally : boardCharacters(self.getOwner(), true)){
                    ally->getbuff().addBuff(temporaryCritChanceBuff(50, 6.0f));
                }
                *used = true;
            }
        });
    } else if(skillName == "equip_taiyang_qiyuan"){
        created->setAfterAttack([](effect& self, AttackContext& context) {
            if(context.attacker == self.getOwner()){
                healPercentMaxHp(self.getOwner(), lowestHpAlly(self.getOwner()), 5);
            }
        });
    } else if(skillName == "equip_yuguang_qingshen"){
        created->setAfterSkill([](effect& self, SkillContext& context) {
            if(context.caster == self.getOwner()){
                Character* ally = lowestHpAlly(self.getOwner());
                if(ally){
                    ally->getbuff().addBuff(temporaryDodgeBuff(50, 4.0f));
                }
            }
        });
    } else if(skillName == "equip_xueling_anhun"){
        created->setAfterSkill([](effect& self, SkillContext& context) {
            if(context.caster == self.getOwner()){
                healPercentMaxHp(self.getOwner(), lowestHpAlly(self.getOwner()), 12);
            }
        });
    } else if(skillName == "equip_yuexia_shouhu"){
        auto used = std::make_shared<bool>(false);
        created->setAfterBeAttacked([used](effect& self, BeAttackedContext& context) {
            Character* owner = self.getOwner();
            if(owner && context.target == owner && !*used &&
               owner->getstats().getHP() * 100 < owner->getstats().getMAXHP() * 40){
                teleportToRandomPassable(owner);
                owner->getstats().heal(owner->getstats().getMAXHP(), owner);
                *used = true;
            }
        });
    } else if(skillName == "equip_shengshu_ganlu"){
        created->setBeforeHeal([](effect& self, HealContext& context) {
            if(context.source == self.getOwner() && context.target && context.target != self.getOwner()){
                context.target->getbuff().addBuff(temporaryAttackPercentBuff(30, 3.0f));
            }
        });
    } else if(skillName == "equip_wangguan_zhufu"){
        created->setOnBattleStart([](effect& self, BattleContext& context) {
            if(context.character == self.getOwner()){
                Character* ally = lowestHpAlly(self.getOwner());
                if(ally){
                    ally->getstats().modifySHIELF(std::max(1, ally->getstats().getMAXHP() * 40 / 100));
                }
            }
        });
    } else if(skillName == "equip_luhuo_fanji"){
        created->setAfterBeAttacked([](effect& self, BeAttackedContext& context) {
            if(context.target == self.getOwner() && context.attacker && !context.dodged){
                context.attacker->getbuff().addBuff(burningPercentBuff(self.getOwner(), 3, 3.0f, 1));
            }
        });
    } else if(skillName == "equip_fengbi"){
        created->setAfterBeAttacked([](effect& self, BeAttackedContext& context) {
            if(context.target == self.getOwner() && context.attacker && !context.dodged){
                pushAttackerAway(self.getOwner(), context.attacker);
            }
        });
    } else if(skillName == "equip_bingchuan_bilei"){
        auto used = std::make_shared<bool>(false);
        created->setAfterBeAttacked([used](effect& self, BeAttackedContext& context) {
            Character* owner = self.getOwner();
            if(owner && context.target == owner && !*used &&
               owner->getstats().getHP() * 100 < owner->getstats().getMAXHP() * 50){
                owner->getstats().modifySHIELF(std::max(1, owner->getstats().getMAXHP() * 30 / 100));
                *used = true;
            }
        });
    } else if(skillName == "equip_yueshi_gedang"){
        created->setAfterBeAttacked([](effect& self, BeAttackedContext& context) {
            if(context.target == self.getOwner() && !context.dodged){
                self.getOwner()->getstats().modifyATTACL(std::max(1, self.getOwner()->getstats().getATTACK() * 5 / 100));
            }
        });
    } else if(skillName == "equip_xiangxin_genxu"){
        created->setOnBattleStart([](effect& self, BattleContext& context) {
            Character* owner = self.getOwner();
            if(context.character == owner){
                Character* ally = lowestMaxHpAlly(owner);
                if(ally && ally->getstats().getMAXHP() < owner->getstats().getMAXHP()){
                    const int delta = owner->getstats().getMAXHP() - ally->getstats().getMAXHP();
                    ally->getstats().modifyMAXHP(delta);
                    ally->getstats().modifyHP(delta);
                }
            }
        });
    } else if(skillName == "equip_wangguan_shouhu"){
        created->setOnBattleStart([](effect& self, BattleContext& context) {
            if(context.character == self.getOwner()){
                Character* ally = nearestAlly(self.getOwner());
                if(ally){
                    ally->getbuff().addBuff(crownGuardianProtectBuff(self.getOwner()));
                }
            }
        });
    } else {
        return nullptr;
    }

    return created;
}

static std::unique_ptr<buff> equipmentModifierBuff(const Equipment& equipment) {
    std::vector<std::unique_ptr<effect>> effects;
    for (const EquipmentModifier& modifier : equipment.modifiers()) {
        effects.push_back(equipmentModifierEffect(modifier));
    }
    if (std::unique_ptr<effect> skillEffect = equipmentSkillEffect(equipment.skillName())) {
        effects.push_back(std::move(skillEffect));
    }
    if (effects.empty()) {
        return nullptr;
    }
    return std::make_unique<buff>(400,
                                  QString::fromStdString(equipment.title()),
                                  -1,
                                  0.0f,
                                  -1,
                                  BuffTrigger::None,
                                  std::move(effects),
                                  1);
}

static Skill equipmentActiveSkillFromName(const std::string& skillName) {
    if(skillName == "equip_yuexiang_lunzhuan"){
        auto fullMoonNext = std::make_shared<bool>(true);
        return {[fullMoonNext](Character* caster, Character*) {
            if(caster == nullptr){
                return;
            }
            if(*fullMoonNext){
                caster->getbuff().addBuff(moonFullAttackBuff());
            }else{
                healPercentMaxHp(caster, lowestHpAlly(caster), 20);
            }
            *fullMoonNext = !*fullMoonNext;
        }, false};
    }
    return {};
}

std::unique_ptr<buff> createEquipmentBuff(const Equipment& equipment) {
    return equipmentModifierBuff(equipment);
}

void addEquipmentStartBuffs(Character* character, const Equipment& equipment) {
    if(character == nullptr){
        return;
    }

    for(int buffId : equipment.buffIds()){
        std::unique_ptr<buff> created = bufffactory::create(buffId, -1);
        if(created){
            character->getbuff().addBuff(std::move(created));
        }
    }
    if(std::unique_ptr<buff> created = createEquipmentBuff(equipment)){
        character->getbuff().addBuff(std::move(created));
    }
}

void addEquipmentActiveSkills(Character* character, const Equipment& equipment) {
    if(character == nullptr){
        return;
    }

    Skill activeSkill = equipmentActiveSkillFromName(equipment.skillName());
    if(activeSkill.handler){
        character->getattack().addSkill(std::move(activeSkill));
    }
}

} // namespace equipskill
