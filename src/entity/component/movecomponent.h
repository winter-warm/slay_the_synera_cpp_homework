#ifndef MOVECOMPONENT_H
#define MOVECOMPONENT_H
#include "component.h"
#include <vector>
#include <functional>
#include <utility>
#include "../../core/board.h"

using TargetSelector = std::function<Character*(Character* self, const Board& board)>;
using PathFinder = std::function<std::vector<Hex>(Character* self, Character* target, const Board& board)>;

Character* nearestEnemy(Character* self, const Board& board);
Character* highestAttackEnemy(Character* self, const Board& board);
std::vector<Hex> Astar(Character* self, Character* target, const Board& board);
std::vector<Hex> flyToAttackPosition(Character* self, Character* target, const Board& board);

class MoveComponent:public component {
private:
    Character* target = nullptr;
    std::vector<Hex> path;
    TargetSelector targetSelector;
    PathFinder pathFinder;
public:
    MoveComponent(Character* owner, PathFinder pf = Astar, TargetSelector ts = nearestEnemy):component(owner), targetSelector(ts), pathFinder(pf){}
    MoveComponent(const MoveComponent& other, Character* owner);

    bool update(Board& board);
    Character* gettar(){return target;}
    void setTargetSelector(TargetSelector selector){targetSelector = std::move(selector); target = nullptr; path.clear();}
    void setPathFinder(PathFinder finder){pathFinder = std::move(finder); path.clear();}
};

#endif // MOVECOMPONENT_H
