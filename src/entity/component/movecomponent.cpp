#include "movecomponent.h"
#include <QQueue>
#include <QSet>
#include "../character/character.h"
#include "teamcomponent.h"
#include <algorithm>
#include <unordered_map>

static bool canAttackFrom(Character* self, const Board& board, const Hex& from, const Hex& targetHex){
    return self && board.dist(from, targetHex) <= self->getstats().getRANGE();
}

MoveComponent::MoveComponent(const MoveComponent& other, Character* owner):component(other, owner) {
    path = other.path; targetSelector = other.targetSelector;
    pathFinder = other.pathFinder; target = other.target;
    previousTargetSelector = other.previousTargetSelector;
    previousPathFinder = other.previousPathFinder;
    restoreTargetSelectorAfterSelection = other.restoreTargetSelectorAfterSelection;
    restorePathFinderAfterPath = other.restorePathFinderAfterPath;
    moveLocks = other.moveLocks;
}

bool MoveComponent::update(Board& board) {
    if(moveLocks > 0) {
        path.clear();
        return false;
    }

    Hex selfHex;
    if(!board.posOf(owner, &selfHex)) {
        target = nullptr;
        path.clear();
        return false;
    }

    Hex targetHex;
    if(target == nullptr ||
       !target->isTargetable() ||
       target->getteam().getteam() == owner->getteam().getteam() ||
       !board.posOf(target, &targetHex) ||
       target->isdead()) {
        target = targetSelector(owner, board);
        if(restoreTargetSelectorAfterSelection) {
            targetSelector = std::move(previousTargetSelector);
            previousTargetSelector = nullptr;
            restoreTargetSelectorAfterSelection = false;
        }
        path.clear();
    }
    if(target == nullptr || !board.posOf(target, &targetHex))return false;
    if(canAttackFrom(owner, board, selfHex, targetHex))return false;

    path = pathFinder(owner, target, board);
    if(restorePathFinderAfterPath) {
        pathFinder = std::move(previousPathFinder);
        previousPathFinder = nullptr;
        restorePathFinderAfterPath = false;
    }
    if(path.empty())return false;

    const Hex next = path.front();
    if(board.move(owner, next)) {
        path.erase(path.begin());
        return true;
    }

    path.clear();
    return false;
}

void MoveComponent::setTargetSelector(TargetSelector selector) {
    targetSelector = std::move(selector);
    previousTargetSelector = nullptr;
    restoreTargetSelectorAfterSelection = false;
    invalidateTargeting();
}

void MoveComponent::setTargetSelectorOnce(TargetSelector selector) {
    previousTargetSelector = targetSelector;
    targetSelector = std::move(selector);
    restoreTargetSelectorAfterSelection = true;
    invalidateTargeting();
}

void MoveComponent::setPathFinder(PathFinder finder) {
    pathFinder = std::move(finder);
    previousPathFinder = nullptr;
    restorePathFinderAfterPath = false;
    invalidatePath();
}

void MoveComponent::setPathFinderOnce(PathFinder finder) {
    previousPathFinder = pathFinder;
    pathFinder = std::move(finder);
    restorePathFinderAfterPath = true;
    invalidatePath();
}

Character* nearestEnemy(Character* self, const Board& board) {
    Hex start;
    QSet<Hex> seen;
    QQueue<Hex> queue;
    if(!board.posOf(self, &start)) {
        return nullptr;
    }

    queue.enqueue(start);
    seen.insert(start);

    while(!queue.isEmpty()) {
        Hex cur = queue.dequeue();
        Unit* unit = board.unitAt(cur);
        Character* tar = dynamic_cast<Character*>(unit);
        if(tar && tar != self && tar->isTargetable() && tar->getteam().getteam() != self->getteam().getteam()) {
            return tar;
        }

        for(const Hex& next: board.neighbors(cur)) {
            if(seen.contains(next))continue;
            seen.insert(next);

            Unit* nextUnit = board.unitAt(next);
            Character* nextTar = dynamic_cast<Character*>(nextUnit);
            if(nextTar && nextTar != self && nextTar->isTargetable() && nextTar->getteam().getteam() != self->getteam().getteam()) {
                return nextTar;
            }
            if(nextUnit)continue;
            if(!board.passable(next))continue;
            queue.enqueue(next);
        }
    }
    return nullptr;
}

Character* highestAttackEnemy(Character* self, const Board& board) {
    Character* best = nullptr;
    for(Unit* unit : board.units()) {
        Character* candidate = dynamic_cast<Character*>(unit);
        if(!candidate ||
           candidate == self ||
           !candidate->isTargetable() ||
           candidate->isdead() ||
           candidate->getteam().getteam() == self->getteam().getteam()) {
            continue;
        }
        if(best == nullptr || candidate->getstats().getATTACK() > best->getstats().getATTACK()) {
            best = candidate;
        }
    }
    return best;
}

Character* lowestHpEnemy(Character* self, const Board& board) {
    Character* best = nullptr;
    for(Unit* unit : board.units()) {
        Character* candidate = dynamic_cast<Character*>(unit);
        if(!candidate ||
           candidate == self ||
           !candidate->isTargetable() ||
           candidate->isdead() ||
           candidate->getteam().getteam() == self->getteam().getteam()) {
            continue;
        }
        if(best == nullptr || candidate->getstats().getHP() < best->getstats().getHP()) {
            best = candidate;
        }
    }
    return best;
}

TargetSelector targetSelectorFromKind(TargetSelectorKind kind) {
    switch(kind) {
    case TargetSelectorKind::NearestEnemy:
        return nearestEnemy;
    case TargetSelectorKind::HighestAttackEnemy:
        return highestAttackEnemy;
    case TargetSelectorKind::LowestHpEnemy:
        return lowestHpEnemy;
    }
    return nearestEnemy;
}

PathFinder pathFinderFromKind(PathFinderKind kind) {
    switch(kind) {
    case PathFinderKind::Astar:
        return Astar;
    case PathFinderKind::FlyToAttackPosition:
        return flyToAttackPosition;
    }
    return Astar;
}

std::vector<Hex> Astar(Character* self, Character* target, const Board& board) {
    Hex start;
    Hex targetHex;
    if(!self || !target || !board.posOf(self, &start) || !board.posOf(target, &targetHex)) {
        return {};
    }
    if(canAttackFrom(self, board, start, targetHex)) {
        return {};
    }

    QSet<Hex> seen;
    QQueue<Hex> queue;
    std::unordered_map<Hex, Hex> parent;
    Hex goal;
    bool found = false;

    queue.enqueue(start);
    seen.insert(start);

    while(!queue.isEmpty() && !found) {
        const Hex cur = queue.dequeue();
        for(const Hex& next : board.neighbors(cur)) {
            if(seen.contains(next))continue;
            if(!board.passable(next))continue;

            seen.insert(next);
            parent.emplace(next, cur);

            if(canAttackFrom(self, board, next, targetHex)) {
                goal = next;
                found = true;
                break;
            }

            queue.enqueue(next);
        }
    }

    if(!found) {
        return {};
    }

    std::vector<Hex> out;
    Hex cur = goal;
    while(!(cur == start)) {
        out.push_back(cur);
        const auto it = parent.find(cur);
        if(it == parent.end()) {
            return {};
        }
        cur = it->second;
    }
    std::reverse(out.begin(), out.end());
    return out;
}

std::vector<Hex> flyToAttackPosition(Character* self, Character* target, const Board& board) {
    Hex start;
    Hex targetHex;
    if(!self || !target || !board.posOf(self, &start) || !board.posOf(target, &targetHex)) {
        return {};
    }
    if(canAttackFrom(self, board, start, targetHex)) {
        return {};
    }

    bool found = false;
    Hex best;
    int bestDist = 0;
    for(const Hex& cell : board.neighbors(targetHex)) {
        if(!board.passable(cell)) {
            continue;
        }
        const int dist = board.dist(start, cell);
        if(!found || dist < bestDist) {
            found = true;
            best = cell;
            bestDist = dist;
        }
    }
    if(!found) {
        for(const Hex& cell : board.cells()) {
            if(!board.passable(cell)) {
                continue;
            }
            if(!canAttackFrom(self, board, cell, targetHex)) {
                continue;
            }
            const int dist = board.dist(start, cell);
            if(!found || dist < bestDist) {
                found = true;
                best = cell;
                bestDist = dist;
            }
        }
    }
    if(!found) {
        return {};
    }
    return {best};
}
