#include "effect.h"

#include "core/board.h"
#include "entity/character/character.h"
#include "entity/component/movecomponent.h"

#include <algorithm>
#include <random>
#include <vector>

namespace character_effect {
effect::BattleHandler cocoStart();
effect::BeAttackedHandler cocoRetreat();
effect::BattleHandler levitation();
effect::BattleHandler sightInduction();
effect::RemoveHandler clearSightInduction();
effect::AttackHandler removeSightInduction();
effect::BattleHandler simpleSpearTargeting();
effect::AttackHandler firstAttackDouble();
effect::TurnHandler burningPercent();
effect::BeAttackedHandler noahDamageReduction();
effect::TurnHandler noahRegeneration();
effect::DeathHandler deathRewind();
effect::BeAttackedHandler mimicry();
}

static std::mt19937& characterEffectRng()
{
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}

static bool emptyForMove(const Board& board, const Hex& hex)
{
    return board.has(hex) && board.passable(hex);
}

static bool moveToBestRetreat(Character* owner, Character* attacker)
{
    Board* board = owner ? owner->getCurrentBoard() : nullptr;
    Hex ownerHex;
    Hex attackerHex;
    if(!board || !attacker || !board->posOf(owner, &ownerHex) || !board->posOf(attacker, &attackerHex)) {
        return false;
    }

    const int currentDistance = board->dist(ownerHex, attackerHex);
    bool found = false;
    Hex best;
    int bestSelfDistance = 0;
    int bestAttackerDistance = currentDistance;
    std::vector<Hex> candidates = board->neighbors(ownerHex);
    for(const Hex& cell : board->cells()) {
        candidates.push_back(cell);
    }
    for(const Hex& cell : candidates) {
        if(!emptyForMove(*board, cell)) {
            continue;
        }
        const int attackerDistance = board->dist(cell, attackerHex);
        const int selfDistance = board->dist(cell, ownerHex);
        if(attackerDistance <= currentDistance) {
            continue;
        }
        if(!found || selfDistance < bestSelfDistance ||
           (selfDistance == bestSelfDistance && attackerDistance > bestAttackerDistance)) {
            found = true;
            best = cell;
            bestAttackerDistance = attackerDistance;
            bestSelfDistance = selfDistance;
        }
    }
    return found && board->move(owner, best);
}

static bool moveToRandomEmpty(Character* owner)
{
    Board* board = owner ? owner->getCurrentBoard() : nullptr;
    if(!board) {
        return false;
    }

    std::vector<Hex> cells;
    for(const Hex& cell : board->cells()) {
        if(emptyForMove(*board, cell)) {
            cells.push_back(cell);
        }
    }
    if(cells.empty()) {
        return false;
    }

    std::shuffle(cells.begin(), cells.end(), characterEffectRng());
    return board->move(owner, cells.front());
}

effect::BattleHandler character_effect::cocoStart()
{
    return [](effect& self, BattleContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.character != owner) {
            return;
        }
        owner->getstats().modifyRANGE(1);
        owner->getstats().setATTACK(owner->getstats().getATTACK() * 90 / 100);
    };
}

effect::BeAttackedHandler character_effect::cocoRetreat()
{
    return [](effect& self, BeAttackedContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.cancelled || context.target != owner) {
            return;
        }
        moveToBestRetreat(owner, context.attacker);
    };
}

effect::BattleHandler character_effect::levitation()
{
    return [](effect& self, BattleContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.character != owner) {
            return;
        }
        owner->getmove().setPathFinder(flyToAttackPosition);
    };
}

effect::BattleHandler character_effect::sightInduction()
{
    return [](effect& self, BattleContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.character != owner) {
            return;
        }
        owner->addUntargetableLock();
    };
}

effect::RemoveHandler character_effect::clearSightInduction()
{
    return [](effect& self) {
        Character* owner = self.getOwner();
        if(owner == nullptr) {
            return false;
        }
        owner->removeUntargetableLock();
        return true;
    };
}

effect::AttackHandler character_effect::removeSightInduction()
{
    return [](effect& self, AttackContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.cancelled || context.attacker != owner) {
            return;
        }
        owner->getbuff().requestRemoveByTemplateId(306);
    };
}

effect::BattleHandler character_effect::simpleSpearTargeting()
{
    return [](effect& self, BattleContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.character != owner) {
            return;
        }
        owner->getmove().setTargetSelector(highestAttackEnemy);
    };
}

effect::AttackHandler character_effect::firstAttackDouble()
{
    return [](effect& self, AttackContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.cancelled || context.attacker != owner) {
            return;
        }
        context.damage *= 2;
    };
}

effect::TurnHandler character_effect::burningPercent()
{
    return [](effect& self, TurnContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.character != owner) {
            return;
        }
        owner->getstats().beattacked(std::max(1, owner->getstats().getMAXHP() * 5 / 100));
    };
}

effect::BeAttackedHandler character_effect::noahDamageReduction()
{
    return [](effect& self, BeAttackedContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.cancelled || context.target != owner) {
            return;
        }
        context.damage /= 2;
    };
}

effect::TurnHandler character_effect::noahRegeneration()
{
    return [](effect& self, TurnContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.character != owner) {
            return;
        }
        owner->getstats().heal(std::max(1, owner->getstats().getMAXHP() / 100));
    };
}

effect::DeathHandler character_effect::deathRewind()
{
    return [](effect& self, DeathContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.cancelled || context.target != owner) {
            return;
        }
        owner->getstats().modifyMAXHP(-owner->getstats().getMAXHP() / 2);
        owner->getstats().heal(owner->getstats().getMAXHP());
        moveToRandomEmpty(owner);
        context.cancelled = true;
    };
}

effect::BeAttackedHandler character_effect::mimicry()
{
    return [](effect& self, BeAttackedContext& context) {
        Character* owner = self.getOwner();
        if(owner == nullptr || context.cancelled || context.target != owner || context.attacker == nullptr) {
            return;
        }
        owner->getstats().setATTACK(context.attacker->getstats().getATTACK());
    };
}
