#include "defaultskill.h"
#include "character.h"
#include "combat/buff/buff.h"
#include "core/board.h"
#include "combat/buff/bufffactory.h"
#include "combat/effect/effect.h"
#include "entity/character/characterfactory.h"
#include "entity/component/movecomponent.h"

#include <QDebug>
#include <algorithm>
#include <memory>
#include <random>
#include <vector>

static Skill enemySkillFromName(const QString& name, bool isBuff);
static Skill tftSkillFromName(const QString& name, bool isBuff);

Skill skillFromName(const QString& name, bool isBuff)
{
    if(name == "ReverseFuture"){return {ReverseFuture, isBuff};}
    else if(name == "Savage"){return {Savage, isBuff};}
    else if(name == "PawnAdvance"){return {PawnAdvance, isBuff};}
    else if(name == "CastleWall"){return {CastleWall, isBuff};}
    else if(name == "KnightFork"){return {KnightFork, isBuff};}
    else if(name == "DiagonalSmite"){return {DiagonalSmite, isBuff};}
    else if(name == "RoyalSweep"){return {RoyalSweep, isBuff};}
    else if(name == "KingCommand"){return {KingCommand, isBuff};}
    else if(name == "KillingIntentSense"){return {KillingIntentSense, isBuff};}
    else if(name == "EhEhOh"){return {EhEhOh, isBuff};}
    else if(name == "Levitation"){return {Levitation, isBuff};}
    else if(name == "SightInduction"){return {SightInduction, isBuff};}
    else if(name == "SimpleSpear"){return {SimpleSpear, isBuff};}
    else if(name == "Fireball"){return {Fireball, isBuff};}
    else if(name == "Kotodama"){return {Kotodama, isBuff};}
    else if(name == "LiquidManipulation"){return {LiquidManipulation, isBuff};}
    else if(name == "Gunshot"){return {Gunshot, isBuff};}
    else if(name == "DeathRewind"){return {DeathRewind, isBuff};}
    else if(name == "Mimicry"){return {Mimicry, isBuff};}
    else if(name == "WitchKiller"){return {WitchKiller, isBuff};}

    Skill tftSkill = tftSkillFromName(name, isBuff);
    if(tftSkill.handler){
        return tftSkill;
    }

    Skill enemySkill = enemySkillFromName(name, isBuff);
    if(enemySkill.handler){
        return enemySkill;
    }

    qDebug() << "Unknown skill name:" << name;
    return {};
}

static bool invalidSkillTarget(Character* caster, Character* target){
    return caster == nullptr || target == nullptr;
}

static std::mt19937& defaultSkillRng()
{
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}



void ReverseFuture(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getbuff().addBuff(bufffactory::create(1, -1));
}

void Savage(Character* caster,Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(25);
}

void PawnAdvance(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(18, caster);
}

void CastleWall(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    caster->getstats().modifySHIELF(18);
    target->getstats().beattacked(12, caster);
}

void KnightFork(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(28, caster);
}

void DiagonalSmite(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(16, caster);
    target->getbuff().addBuff(bufffactory::create(2, 2));
}

void RoyalSweep(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(34, caster);
    caster->getstats().modifyHP(-8);
}

void KingCommand(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    caster->getstats().heal(18, caster);
    caster->getstats().modifySHIELF(12);
    target->getstats().beattacked(14, caster);
}

void KillingIntentSense(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(301, -1));
}

void EhEhOh(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(target->getstats().getHP(), caster);
}

void Levitation(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(305, -1));
}

void SightInduction(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(306, 15));
}

void SimpleSpear(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(308, -1));
    caster->getbuff().addBuff(bufffactory::create(309, -1));
}

void Fireball(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(10, caster);
    target->getbuff().addBuff(bufffactory::create(310, 3));
}

void Kotodama(Character* caster, Character* target){
    if(caster == nullptr){
        return;
    }

    Board* board = caster->getCurrentBoard();
    Character* selected = board ? nearestEnemy(caster, *board) : target;
    if(selected == nullptr){
        return;
    }

    const int buffId = selected->getteam().getteam() == teams::enemy ? 201 : 202;
    selected->getbuff().addBuff(bufffactory::create(buffId, 10));
}

void LiquidManipulation(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(312, -1));
    caster->getbuff().addBuff(bufffactory::create(313, -1));
}

void Gunshot(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(30, caster);
}

void DeathRewind(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(314, -1));
}

void Mimicry(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(315, -1));
}

void WitchKiller(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr || caster->getCurrentBoard() == nullptr){
        return;
    }

    Board* board = caster->getCurrentBoard();
    std::vector<Character*> characters;
    std::vector<Hex> positions;
    for(Unit* unit : board->units()){
        Character* character = dynamic_cast<Character*>(unit);
        Hex hex;
        if(character && board->posOf(character, &hex)){
            character->getstats().modifySHIELF(-character->getstats().getSHIELD());
            character->getstats().setHP(1);
            characters.push_back(character);
            positions.push_back(hex);
        }
    }
    if(characters.size() < 2){
        return;
    }

    std::shuffle(positions.begin(), positions.end(), defaultSkillRng());
    for(Character* character : characters){
        board->remove(character);
    }
    for(size_t i = 0; i < characters.size(); ++i){
        board->add(characters[i], positions[i]);
    }
}

static bool validCombatPair(Character* caster, Character* target)
{
    return caster && target && target->isTargetable() && !target->isdead();
}

static bool isEnemyOf(Character* self, Character* other)
{
    return self && other && self != other && self->getteam().getteam() != other->getteam().getteam();
}

static std::vector<Character*> charactersOnBoard(Character* owner, bool enemies)
{
    std::vector<Character*> out;
    Board* board = owner ? owner->getCurrentBoard() : nullptr;
    if(!board) {
        return out;
    }

    for(Unit* unit : board->units()) {
        Character* character = dynamic_cast<Character*>(unit);
        if(!character || character == owner || character->isdead() || !character->isTargetable()) {
            continue;
        }
        const bool enemy = character->getteam().getteam() != owner->getteam().getteam();
        if(enemy == enemies) {
            out.push_back(character);
        }
    }
    return out;
}

static Character* lowestHpAlly(Character* owner)
{
    Character* best = owner;
    for(Character* ally : charactersOnBoard(owner, false)) {
        if(!best || ally->getstats().getHP() < best->getstats().getHP()) {
            best = ally;
        }
    }
    return best;
}

static void damageTarget(Character* caster, Character* target, int percent, int flatBonus = 0)
{
    if(!validCombatPair(caster, target)) {
        return;
    }
    const int damage = std::max(1, caster->getstats().getATTACK() * percent / 100 + flatBonus);
    target->getstats().beattacked(damage, caster);
}

static void damageAroundTarget(Character* caster, Character* target, int primaryPercent, int splashPercent)
{
    if(!validCombatPair(caster, target)) {
        return;
    }
    Board* board = caster->getCurrentBoard();
    Hex targetHex;
    if(!board || !board->posOf(target, &targetHex)) {
        damageTarget(caster, target, primaryPercent);
        return;
    }

    damageTarget(caster, target, primaryPercent);
    const int splashDamage = std::max(1, caster->getstats().getATTACK() * splashPercent / 100);
    for(Unit* unit : board->units()) {
        Character* candidate = dynamic_cast<Character*>(unit);
        Hex candidateHex;
        if(!candidate || candidate == target || !isEnemyOf(caster, candidate) ||
           !board->posOf(candidate, &candidateHex) || board->dist(targetHex, candidateHex) > 1) {
            continue;
        }
        candidate->getstats().beattacked(splashDamage, caster);
    }
}

static void healLowestAlly(Character* caster, int percentOfAttack, int minAmount = 1)
{
    Character* ally = lowestHpAlly(caster);
    if(!ally) {
        return;
    }
    ally->getstats().heal(std::max(minAmount, caster->getstats().getATTACK() * percentOfAttack / 100), caster);
}

static void shieldAllies(Character* caster, int amount)
{
    if(!caster) {
        return;
    }
    caster->getstats().modifySHIELF(amount);
    for(Character* ally : charactersOnBoard(caster, false)) {
        ally->getstats().modifySHIELF(amount);
    }
}

static void buffAlliesAttack(Character* caster, int percent)
{
    if(!caster) {
        return;
    }
    auto apply = [percent](Character* character) {
        character->getstats().modifyATTACL(std::max(1, character->getstats().getATTACK() * percent / 100));
    };
    apply(caster);
    for(Character* ally : charactersOnBoard(caster, false)) {
        apply(ally);
    }
}

static void buffAlliesDefense(Character* caster, int amount)
{
    if(!caster) {
        return;
    }
    caster->getstats().modifyDEFESE(amount);
    for(Character* ally : charactersOnBoard(caster, false)) {
        ally->getstats().modifyDEFESE(amount);
    }
}

static Character* lowestHpEnemyOnBoard(Character* owner)
{
    Character* best = nullptr;
    for(Character* enemy : charactersOnBoard(owner, true)) {
        if(!best || enemy->getstats().getHP() < best->getstats().getHP()) {
            best = enemy;
        }
    }
    return best;
}

static Character* secondEnemyOnBoard(Character* owner, Character* first)
{
    for(Character* enemy : charactersOnBoard(owner, true)) {
        if(enemy != first) {
            return enemy;
        }
    }
    return nullptr;
}

static void damageAllEnemies(Character* caster, int percent)
{
    for(Character* enemy : charactersOnBoard(caster, true)) {
        damageTarget(caster, enemy, percent);
    }
}

static void reduceTargetAttack(Character* target, int percent)
{
    if(!target || percent <= 0) {
        return;
    }
    target->getstats().modifyATTACL(-std::max(1, target->getstats().getATTACK() * percent / 100));
}

static std::unique_ptr<buff> singleEffectBuff(int id,
                                              const QString& name,
                                              float duration,
                                              float cooldown,
                                              BuffTrigger trigger,
                                              std::unique_ptr<effect> eff,
                                              int maxTriggers = -1)
{
    std::vector<std::unique_ptr<effect>> effects;
    effects.push_back(std::move(eff));
    return std::make_unique<buff>(id, name, duration, cooldown, maxTriggers, trigger, std::move(effects));
}

static void addControlBuff(Character* target, float duration, bool lockMove, bool lockAction)
{
    if(!target) {
        return;
    }
    auto control = std::make_unique<effect>(700, "enemy_control");
    control->setApply([lockMove, lockAction](effect&, Character* owner) {
        if(!owner) {
            return false;
        }
        if(lockMove) {
            owner->getmove().addMoveLock();
        }
        if(lockAction) {
            owner->getattack().addActionLock();
        }
        return true;
    });
    control->setRemove([lockMove, lockAction](effect& self) {
        Character* owner = self.getOwner();
        if(!owner) {
            return false;
        }
        if(lockMove) {
            owner->getmove().removeMoveLock();
        }
        if(lockAction) {
            owner->getattack().removeActionLock();
        }
        return true;
    });
    target->getbuff().addBuff(singleEffectBuff(700, QString::fromUtf8("Control"), duration, 0.0f,
                                               BuffTrigger::None, std::move(control)));
}

static void summonNear(Character* caster, int templateId, int count)
{
    Board* board = caster ? caster->getCurrentBoard() : nullptr;
    Hex origin;
    if(!board || !board->posOf(caster, &origin)) {
        return;
    }

    std::vector<Hex> cells = board->neighbors(origin);
    for(const Hex& cell : board->cells()) {
        cells.push_back(cell);
    }

    int summonedCount = 0;
    for(const Hex& cell : cells) {
        if(summonedCount >= count || !board->passable(cell)) {
            continue;
        }
        std::unique_ptr<Character> summoned = characterfactory::create(templateId);
        if(!summoned) {
            return;
        }
        Character* raw = summoned.get();
        raw->getattack().activateAndRemoveBuffSkills();
        std::unique_ptr<Unit> unit(std::move(summoned));
        if(board->addOwned(std::move(unit), cell)) {
            raw->setCurrentBoard(board);
            raw->onBattleStart();
            ++summonedCount;
        }
    }
}

static Skill addBattleBuffSkill(std::function<void(Character*)> apply)
{
    return { [apply = std::move(apply)](Character* caster, Character*) {
        if(!caster) {
            return;
        }
        auto eff = std::make_unique<effect>(701, "enemy_battle_start");
        eff->setOnBattleStart([apply](effect& self, BattleContext& context) {
            Character* owner = self.getOwner();
            if(owner && context.character == owner) {
                apply(owner);
            }
        });
        caster->getbuff().addBuff(singleEffectBuff(701, QString::fromUtf8("Battle Start"), -1.0f, 0.0f,
                                                   BuffTrigger::OnBattleStart, std::move(eff), 1));
    }, true };
}

static Skill addBeforeAttackSkill(std::function<void(Character*, AttackContext&)> apply)
{
    return { [apply = std::move(apply)](Character* caster, Character*) {
        if(!caster) {
            return;
        }
        auto eff = std::make_unique<effect>(702, "enemy_before_attack");
        eff->setBeforeAttack([apply](effect& self, AttackContext& context) {
            Character* owner = self.getOwner();
            if(owner && context.attacker == owner && !context.cancelled) {
                apply(owner, context);
            }
        });
        caster->getbuff().addBuff(singleEffectBuff(702, QString::fromUtf8("Before Attack"), -1.0f, 0.0f,
                                                   BuffTrigger::None, std::move(eff)));
    }, true };
}

static Skill addAfterAttackSkill(std::function<void(Character*, AttackContext&)> apply)
{
    return { [apply = std::move(apply)](Character* caster, Character*) {
        if(!caster) {
            return;
        }
        auto eff = std::make_unique<effect>(703, "enemy_after_attack");
        eff->setAfterAttack([apply](effect& self, AttackContext& context) {
            Character* owner = self.getOwner();
            if(owner && context.attacker == owner && !context.cancelled) {
                apply(owner, context);
            }
        });
        caster->getbuff().addBuff(singleEffectBuff(703, QString::fromUtf8("After Attack"), -1.0f, 0.0f,
                                                   BuffTrigger::None, std::move(eff)));
    }, true };
}

static Skill addAfterBeAttackedSkill(std::function<void(Character*, BeAttackedContext&)> apply)
{
    return { [apply = std::move(apply)](Character* caster, Character*) {
        if(!caster) {
            return;
        }
        auto eff = std::make_unique<effect>(704, "enemy_after_be_attacked");
        eff->setAfterBeAttacked([apply](effect& self, BeAttackedContext& context) {
            Character* owner = self.getOwner();
            if(owner && context.target == owner && !context.cancelled) {
                apply(owner, context);
            }
        });
        caster->getbuff().addBuff(singleEffectBuff(704, QString::fromUtf8("After Hit"), -1.0f, 0.0f,
                                                   BuffTrigger::None, std::move(eff)));
    }, true };
}

static Skill addDeathSkill(std::function<void(Character*, DeathContext&)> apply, int maxTriggers = 1)
{
    return { [apply = std::move(apply), maxTriggers](Character* caster, Character*) {
        if(!caster) {
            return;
        }
        auto eff = std::make_unique<effect>(705, "enemy_before_death");
        eff->setBeforeDeath([apply](effect& self, DeathContext& context) {
            Character* owner = self.getOwner();
            if(owner && context.target == owner && !context.cancelled) {
                apply(owner, context);
            }
        });
        caster->getbuff().addBuff(singleEffectBuff(705, QString::fromUtf8("Death Trigger"), -1.0f, 0.0f,
                                                   BuffTrigger::BeforeDeath, std::move(eff), maxTriggers));
    }, true };
}

static Skill addTickSkill(float interval, std::function<void(Character*)> apply)
{
    return { [interval, apply = std::move(apply)](Character* caster, Character*) {
        if(!caster) {
            return;
        }
        auto eff = std::make_unique<effect>(706, "enemy_tick");
        eff->setOnTurnStart([apply](effect& self, TurnContext& context) {
            Character* owner = self.getOwner();
            if(owner && context.character == owner) {
                apply(owner);
            }
        });
        caster->getbuff().addBuff(singleEffectBuff(706, QString::fromUtf8("Tick"), -1.0f, interval,
                                                   BuffTrigger::None, std::move(eff)));
    }, true };
}

static Skill nthAttackSkill(int every, int extraPercent)
{
    return addBeforeAttackSkill([count = std::make_shared<int>(0), every, extraPercent](Character*, AttackContext& context) {
        ++(*count);
        if(every > 0 && *count % every == 0) {
            context.damage += context.damage * extraPercent / 100;
        }
    });
}

static Skill tftSkillFromName(const QString& name, bool isBuff)
{
    (void)isBuff;
    if(name == "FrierenAnalysis") {
        return {[](Character* caster, Character* target) {
            if(!validCombatPair(caster, target)) { return; }
            damageTarget(caster, target, 180);
            if(target->getstats().getDEFENSE() < caster->getstats().getDEFENSE()) {
                damageTarget(caster, target, 80);
            }
        }, false};
    }
    if(name == "FernRapidBolt") { return {[](Character* caster, Character* target) { damageTarget(caster, target, 90); damageTarget(caster, target, 70); }, false}; }
    if(name == "StarkAxeGuard") { return {[](Character* caster, Character* target) { damageTarget(caster, target, 160); if(caster) caster->getstats().modifyDEFESE(15); }, false}; }
    if(name == "HimmelHarvest") {
        return {[](Character* caster, Character* target) {
            Character* selected = lowestHpEnemyOnBoard(caster);
            if(!selected) { selected = target; }
            const int before = selected ? selected->getstats().getHP() : 0;
            damageTarget(caster, selected, 200);
            if(caster && selected && before > 0 && selected->isdead()) {
                caster->getstats().modifyMP(std::max(1, caster->getstats().getMAXMP() / 2));
            }
        }, false};
    }
    if(name == "AuraScale") {
        return {[](Character* caster, Character* target) {
            if(!validCombatPair(caster, target)) { return; }
            if(target->getstats().getDEFENSE() < caster->getstats().getDEFENSE()) {
                reduceTargetAttack(target, 80);
            } else {
                damageTarget(caster, target, 200);
            }
        }, false};
    }
    if(name == "AquaPurify") { return {[](Character* caster, Character*) { healLowestAlly(caster, 120, 10); }, false}; }
    if(name == "MeguminExplosion") {
        return {[](Character* caster, Character*) {
            damageAllEnemies(caster, 250);
            addControlBuff(caster, 2.0f, false, true);
        }, false};
    }
    if(name == "DarknessTaunt") { return {[](Character* caster, Character*) { if(caster) { caster->getstats().modifyDEFESE(30); caster->getstats().modifySHIELF(35); } }, false}; }
    if(name == "KazumaSteal") {
        return addBeforeAttackSkill([](Character* owner, AttackContext& context) {
            if(context.target) {
                const int stolen = std::max(1, context.target->getstats().getATTACK() / 10);
                context.target->getstats().modifyATTACL(-stolen);
                owner->getstats().modifyATTACL(stolen);
            }
        });
    }
    if(name == "ChrisBackstab") {
        return {[](Character* caster, Character* target) {
            int percent = 170;
            if(caster && target && target->getstats().getHP() > caster->getstats().getHP()) {
                percent += 40;
            }
            damageTarget(caster, target, percent);
        }, false};
    }
    if(name == "JotaroTimeStop" || name == "DioTheWorld") {
        const int percent = name == "DioTheWorld" ? 250 : 220;
        return { [percent](Character* caster, Character* target) { addControlBuff(target, 1.5f, true, true); damageTarget(caster, target, percent); }, false };
    }
    if(name == "JosephRippleCounter") {
        return addAfterBeAttackedSkill([](Character* owner, BeAttackedContext&) {
            const int lostHp = std::max(1, owner->getstats().getMAXHP() - owner->getstats().getHP());
            Character* target = lowestHpEnemyOnBoard(owner);
            if(target) {
                target->getstats().beattacked(lostHp, owner);
            }
        });
    }
    if(name == "KakyoinEmeraldSplash") { return {[](Character* caster, Character* target) { for(int i = 0; i < 5; ++i) { damageTarget(caster, target, 70); } }, false}; }
    if(name == "BucciaratiZipper") { return {[](Character* caster, Character* target) { damageTarget(caster, target, 160); addControlBuff(target, 2.0f, true, true); }, false}; }
    if(name == "KiraKillerQueen") { return {[](Character* caster, Character* target) { damageAroundTarget(caster, target, 260, 130); }, false}; }
    if(name == "RoyalGuardBlock") { return {[](Character* caster, Character*) { if(caster) { caster->getstats().modifyDEFESE(20); caster->getstats().modifySHIELF(25); } }, false}; }
    if(name == "ValkyrieWhirlwind") { return {[](Character* caster, Character* target) { damageAroundTarget(caster, target, 140, 140); }, false}; }
    if(name == "MiniPekkaHeavySlash") { return {[](Character* caster, Character* target) { damageTarget(caster, target, 250); if(caster) caster->getstats().modifyATTACL(-std::max(1, caster->getstats().getATTACK() * 30 / 100)); }, false}; }
    if(name == "SkeletonArmySummon") { return {[](Character* caster, Character* target) { damageAroundTarget(caster, target, 110, 70); if(caster) caster->getstats().modifySHIELF(18); }, false}; }
    if(name == "BombTowerBlast") { return addBeforeAttackSkill([](Character* owner, AttackContext& context) { context.damage += context.damage / 5; damageAroundTarget(owner, context.target, 0, 60); }); }
    if(name == "TombstoneSummon") { return addTickSkill(5.0f, [](Character* owner) { shieldAllies(owner, 10); }); }
    if(name == "TeslaArc") {
        return addBeforeAttackSkill([](Character* owner, AttackContext& context) {
            Character* second = secondEnemyOnBoard(owner, context.target);
            if(second) {
                damageTarget(owner, second, 80);
            }
        });
    }
    if(name == "GoblinHutSpawn") { return addTickSkill(8.0f, [](Character* owner) { buffAlliesAttack(owner, 8); shieldAllies(owner, 8); }); }
    if(name == "ElixirCollectorBlessing") { return addTickSkill(5.0f, [](Character* owner) { buffAlliesAttack(owner, 15); buffAlliesDefense(owner, 3); }); }
    if(name == "RegentForgeBlade") { return {[](Character* caster, Character*) { buffAlliesAttack(caster, 12); }, false}; }
    if(name == "IroncladBloodBurn") { return {[](Character* caster, Character*) { if(caster) { caster->getstats().modifyATTACL(std::max(1, caster->getstats().getATTACK() * 30 / 100)); caster->getstats().modifySHIELF(20); } }, false}; }
    if(name == "SilentHunterKnives") { return {[](Character* caster, Character*) { damageAllEnemies(caster, 70); }, false}; }
    if(name == "DefectRobotStartup") { return {[](Character* caster, Character*) { damageAllEnemies(caster, 999); }, false}; }
    if(name == "WatcherStance") { return {[](Character* caster, Character*) { if(caster) { caster->getstats().modifyATTACL(std::max(1, caster->getstats().getATTACK() * 35 / 100)); caster->getstats().modifyDEFESE(std::max(1, caster->getstats().getDEFENSE() * 35 / 100)); caster->getstats().heal(30, caster); } }, false}; }
    if(name == "AmiyaBurst") {
        return {[](Character* caster, Character* target) {
            int percent = 190;
            if(target && target->getstats().getHP() * 100 <= target->getstats().getMAXHP() * 40) {
                percent += 50;
            }
            damageTarget(caster, target, percent);
        }, false};
    }
    if(name == "ChenChixiao") { return {[](Character* caster, Character* target) { damageTarget(caster, target, 100); }, false}; }
    if(name == "ExusiaiOverload") { return {[](Character* caster, Character* target) { for(int i = 0; i < 4; ++i) { damageTarget(caster, target, 80); } }, false}; }
    if(name == "SilverashTruesilver") { return {[](Character* caster, Character*) { damageAllEnemies(caster, 180); }, false}; }
    if(name == "SariaHealField") { return {[](Character* caster, Character*) { healLowestAlly(caster, 150, 20); buffAlliesDefense(caster, 4); }, false}; }
    if(name == "NightingaleSanctuary") { return {[](Character* caster, Character*) { healLowestAlly(caster, 120, 18); shieldAllies(caster, 15); }, false}; }
    if(name == "MelanthaSwordBoost") { return {[](Character* caster, Character*) { if(caster) caster->getstats().modifyATTACL(std::max(1, caster->getstats().getATTACK() * 40 / 100)); }, false}; }
    if(name == "KroosCrit") { return {[](Character* caster, Character* target) { damageTarget(caster, target, 200); }, false}; }
    if(name == "AnselHeal") { return {[](Character* caster, Character*) { healLowestAlly(caster, 120, 12); }, false}; }
    if(name == "LavaBlast") { return {[](Character* caster, Character* target) { damageAroundTarget(caster, target, 140, 90); }, false}; }
    return {};
}

static Skill enemySkillFromName(const QString& name, bool isBuff)
{
    (void)isBuff;
    if(name == "StreetBrawl") { return nthAttackSkill(4, 40); }
    if(name == "ShieldBreakCounter") {
        return addAfterBeAttackedSkill([](Character* owner, BeAttackedContext& context) {
            if(context.attacker) {
                context.attacker->getstats().beattacked(std::max(1, owner->getstats().getATTACK() * 80 / 100), owner);
            }
        });
    }
    if(name == "TableGuard") { return {[](Character* caster, Character*) { if(caster) caster->getstats().modifySHIELF(10); }, false}; }
    if(name == "Pickpocket") {
        return {[](Character* caster, Character* target) {
            if(!validCombatPair(caster, target)) { return; }
            const int stolen = std::max(1, target->getstats().getATTACK() * 30 / 100);
            target->getstats().modifyATTACL(-stolen);
            caster->getstats().modifyATTACL(stolen);
        }, false};
    }
    if(name == "FocusShot") {
        return addBeforeAttackSkill([lastTarget = std::make_shared<int>(-1), streak = std::make_shared<int>(0)]
                                    (Character*, AttackContext& context) {
            const int id = context.target ? context.target->id() : -1;
            if(id == *lastTarget) {
                ++(*streak);
            } else {
                *lastTarget = id;
                *streak = 1;
            }
            context.damage += context.damage * std::min(60, (*streak - 1) * 15) / 100;
        });
    }
    if(name == "AdventurerHeat") { return {[](Character* caster, Character* target) { damageTarget(caster, target, 160); }, false}; }
    if(name == "SuppressiveShot") {
        return addBeforeAttackSkill([](Character*, AttackContext& context) {
            if(context.target && context.target->getstats().getHP() * 100 <= context.target->getstats().getMAXHP() * 40) {
                context.damage += context.damage / 2;
            }
        });
    }
    if(name == "TrainingDummy") {
        return addAfterBeAttackedSkill([](Character* owner, BeAttackedContext&) { owner->getstats().modifySHIELF(8); });
    }
    if(name == "ArrowTowerBurst") { return {[](Character* caster, Character* target) { damageTarget(caster, target, 100); }, false}; }
    if(name == "ClockworkOverload" || name == "ClockworkKnightOverload") {
        return addBeforeAttackSkill([](Character* owner, AttackContext& context) {
            if(owner->getstats().getHP() * 100 <= owner->getstats().getMAXHP() * 35) {
                context.damage += context.damage / 2;
            }
        });
    }
    if(name == "CleaningMode") { return {[](Character* caster, Character*) { if(caster) caster->getstats().modifySHIELF(caster->getstats().getSHIELD()); }, false}; }
    if(name == "ForceThrough" || name == "AlleyAmbush") {
        return addBattleBuffSkill([](Character* owner) { owner->getstats().modifyATTACL(std::max(2, owner->getstats().getATTACK() / 4)); });
    }
    if(name == "GuardCaptainSignal" || name == "GoblinCommand" || name == "WolfKingAura" || name == "DragonflameBlessing") {
        return {[](Character* caster, Character*) { buffAlliesAttack(caster, 15); shieldAllies(caster, 8); }, false};
    }
    if(name == "HeavyCannonShot" || name == "GoblinBomb" || name == "PumpkinBomb" || name == "WyrmlingBreath") {
        return {[](Character* caster, Character* target) { damageAroundTarget(caster, target, 140, 70); }, false};
    }
    if(name == "GuardianWall") { return addBattleBuffSkill([](Character* owner) { shieldAllies(owner, 18); }); }
    if(name == "LockShot" || name == "CrystalBeam") { return {[](Character* caster, Character* target) { damageTarget(caster, target, 180); }, false}; }
    if(name == "HealingPrayer" || name == "PollenHeal") { return {[](Character* caster, Character*) { healLowestAlly(caster, 150, 20); }, false}; }
    if(name == "PackHunter") {
        return addBeforeAttackSkill([](Character* owner, AttackContext& context) {
            const int bonus = static_cast<int>(charactersOnBoard(owner, false).size()) * 10;
            context.damage += context.damage * std::min(40, bonus) / 100;
        });
    }
    if(name == "BoarCharge") { return addBattleBuffSkill([](Character* owner) { owner->getstats().modifySHIELF(25); owner->getstats().modifyATTACL(4); }); }
    if(name == "RootSnare" || name == "PortraitStun") { return {[](Character*, Character* target) { addControlBuff(target, 3.0f, true, true); }, false}; }
    if(name == "HiddenShot") {
        return addBeforeAttackSkill([untouched = std::make_shared<bool>(true)](Character*, AttackContext& context) {
            if(*untouched) { context.damage += context.damage / 2; }
        });
    }
    if(name == "SporeHeal") {
        return addDeathSkill([](Character* owner, DeathContext&) {
            for(Character* ally : charactersOnBoard(owner, false)) {
                ally->getstats().heal(std::max(8, owner->getstats().getMAXHP() / 5), owner);
            }
        });
    }
    if(name == "GoblinPlunder") {
        return addBeforeAttackSkill([](Character*, AttackContext& context) {
            if(context.target && context.target->getstats().getHP() * 100 <= context.target->getstats().getMAXHP() * 45) {
                context.damage += context.damage / 2;
            }
        });
    }
    if(name == "GoblinSpearSplash" || name == "GhostPierce") { return {[](Character* caster, Character* target) { damageAroundTarget(caster, target, 110, 45); }, false}; }
    if(name == "GoblinDrums") {
        return addTickSkill(4.0f, [](Character* owner) { buffAlliesAttack(owner, 8); });
    }
    if(name == "ThornArmor") {
        return addAfterBeAttackedSkill([](Character* owner, BeAttackedContext& context) {
            if(context.attacker) { context.attacker->getstats().beattacked(std::max(1, context.damage / 4), owner); }
        });
    }
    if(name == "SlimeSplit") { return addDeathSkill([](Character* owner, DeathContext&) { summonNear(owner, 2012, 1); }); }
    if(name == "GoblinAlchemy" || name == "PotionSlime") {
        return {[](Character* caster, Character*) {
            if(!caster) { return; }
            std::uniform_int_distribution<int> distribution(0, 2);
            const int roll = distribution(defaultSkillRng());
            if(roll == 0) { caster->getstats().heal(caster->getstats().getMAXHP() / 4, caster); }
            else if(roll == 1) { caster->getstats().modifySHIELF(20); }
            else { caster->getstats().modifyATTACL(5); }
        }, false};
    }
    if(name == "AncientGuard" || name == "StoneSkin" || name == "BlackIronGuard") {
        return addAfterBeAttackedSkill([](Character* owner, BeAttackedContext&) { owner->getstats().modifySHIELF(10); });
    }
    if(name == "VineJudgement" || name == "BoneCurse" || name == "DollCurse") {
        return {[](Character*, Character* target) {
            if(target) {
                target->getstats().modifyATTACL(-std::max(1, target->getstats().getATTACK() / 5));
            }
        }, false};
    }
    if(name == "MoonBlessing") {
        return {[](Character* caster, Character*) { healLowestAlly(caster, 120, 25); shieldAllies(caster, 15); summonNear(caster, 2006, 2); }, false};
    }
    if(name == "BoneReform") {
        return addDeathSkill([](Character* owner, DeathContext& context) {
            owner->getstats().heal(owner->getstats().getMAXHP() / 2, owner);
            context.cancelled = true;
        });
    }
    if(name == "BatLifesteal" || name == "BloodHarvest") {
        return addAfterAttackSkill([](Character* owner, AttackContext& context) { owner->getstats().heal(std::max(1, context.damage / 3), owner); });
    }
    if(name == "BroomFlurry") { return nthAttackSkill(3, 80); }
    if(name == "ForbiddenPage") { return {[](Character* caster, Character* target) { damageTarget(caster, target, 175); }, false}; }
    if(name == "MimicChestRage" || name == "GolemCoreRage") {
        return addAfterBeAttackedSkill([](Character* owner, BeAttackedContext&) {
            if(owner->getstats().getHP() * 100 <= owner->getstats().getMAXHP() * 40) {
                owner->getstats().modifyATTACL(3);
                owner->getstats().modifySHIELF(8);
            }
        });
    }
    if(name == "Necromancy") { return {[](Character* caster, Character*) { summonNear(caster, 3001, 3); }, false}; }
    if(name == "BloodMoonExecute") {
        return {[](Character* caster, Character* target) {
            if(!validCombatPair(caster, target)) { return; }
            if(target->getstats().getHP() * 100 <= target->getstats().getMAXHP() * 35) {
                target->getstats().beattacked(target->getstats().getHP(), caster);
            } else {
                damageTarget(caster, target, 160);
            }
        }, false};
    }
    if(name == "DragonApocalypse") { return {[](Character* caster, Character* target) { damageAroundTarget(caster, target, 220, 120); }, false}; }
    if(name == "DuelMe") {
        return addAfterBeAttackedSkill([used = std::make_shared<bool>(false)](Character* owner, BeAttackedContext&) {
            if(*used || owner->getstats().getHP() * 100 > owner->getstats().getMAXHP() * 50) {
                return;
            }
            *used = true;
            Board* board = owner->getCurrentBoard();
            Character* target = board ? lowestHpEnemy(owner, *board) : nullptr;
            if(target) {
                target->getstats().beattacked(target->getstats().getHP(), owner);
            }
        });
    }

    return {};
}
