#include "movecomponent.h"
#include <QQueue>
#include <QSet>
#include "../character/character.h"
#include "teamcomponent.h"

MoveComponent::MoveComponent(const MoveComponent& other, Character* owner):component(other, owner) {
    path = other.path; targetSelector = other.targetSelector;
    pathFinder = other.pathFinder; target = other.target;
}

bool MoveComponent::update(const Board& board) {
    if(target == nullptr) {
        target = targetSelector(owner, board);
    }
    if(target == nullptr)return false;
    return false;
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
        if(tar && tar != self && tar->getteam().getteam() != self->getteam().getteam()) {
            return tar;
        }

        for(const Hex& next: board.neighbors(cur)) {
            if(seen.contains(next))continue;
            seen.insert(next);

            Unit* nextUnit = board.unitAt(next);
            Character* nextTar = dynamic_cast<Character*>(nextUnit);
            if(nextTar && nextTar != self && nextTar->getteam().getteam() != self->getteam().getteam()) {
                return nextTar;
            }
            if(nextUnit)continue;
            if(!board.passable(next))continue;
            queue.enqueue(next);
        }
    }
    return nullptr;
}

std::vector<Hex> Astar(Character* self, Character* target, const Board& board) {
    return {};
}
