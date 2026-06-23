#ifndef MOVECOMPONENT_H
#define MOVECOMPONENT_H
#include "component.h"
#include <vector>
#include <functional>
#include <utility>
#include "../../core/board.h"

using TargetSelector = std::function<Character*(Character* self, const Board& board)>;
using PathFinder = std::function<std::vector<Hex>(Character* self, Character* target, const Board& board)>;

enum class TargetSelectorKind {
    NearestEnemy,
    HighestAttackEnemy,
    LowestHpEnemy
};

enum class PathFinderKind {
    Astar,
    FlyToAttackPosition
};

Character* nearestEnemy(Character* self, const Board& board);
Character* highestAttackEnemy(Character* self, const Board& board);
Character* lowestHpEnemy(Character* self, const Board& board);
TargetSelector targetSelectorFromKind(TargetSelectorKind kind);
PathFinder pathFinderFromKind(PathFinderKind kind);
std::vector<Hex> Astar(Character* self, Character* target, const Board& board);
std::vector<Hex> flyToAttackPosition(Character* self, Character* target, const Board& board);

class MoveComponent:public component {
private:
    Character* target = nullptr;
    std::vector<Hex> path;
    TargetSelector targetSelector;
    PathFinder pathFinder;
    TargetSelector previousTargetSelector;
    PathFinder previousPathFinder;
    bool restoreTargetSelectorAfterSelection = false;
    bool restorePathFinderAfterPath = false;
    void invalidateTargeting(){target = nullptr; path.clear();}
    void invalidatePath(){path.clear();}
public:
    MoveComponent(Character* owner, PathFinder pf = Astar, TargetSelector ts = nearestEnemy):component(owner), targetSelector(ts), pathFinder(pf){}
    MoveComponent(const MoveComponent& other, Character* owner);

    bool update(Board& board);
    Character* gettar(){return target;}
    void setTargetSelector(TargetSelector selector);
    void setTargetSelector(TargetSelectorKind kind){setTargetSelector(targetSelectorFromKind(kind));}
    void setTargetSelectorOnce(TargetSelector selector);
    void setTargetSelectorOnce(TargetSelectorKind kind){setTargetSelectorOnce(targetSelectorFromKind(kind));}
    void setPathFinder(PathFinder finder);
    void setPathFinder(PathFinderKind kind){setPathFinder(pathFinderFromKind(kind));}
    void setPathFinderOnce(PathFinder finder);
    void setPathFinderOnce(PathFinderKind kind){setPathFinderOnce(pathFinderFromKind(kind));}
};

#endif // MOVECOMPONENT_H
