#ifndef WORLD_MAP_MAPNODE_H
#define WORLD_MAP_MAPNODE_H

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

enum class MapNodeType {
    Rest,
    Event,
    NormalBattle,
    EliteBattle
};

enum class MapNodeRole {
    Normal,
    Start,
    End
};

// event_id meanings:
// 0 = start, 1 = boss, 2 = rest, 3 = normal battle,
// 4 = elite battle, 100-399 = other events.
namespace MapEventId {
static constexpr int Start = 0;
static constexpr int Boss = 1;
static constexpr int Rest = 2;
static constexpr int NormalBattle = 3;
static constexpr int EliteBattle = 4;
static constexpr int FirstEvent = 100;
}

struct MapNode {
    int id = -1;
    int event_id = MapEventId::NormalBattle;
    int layerId = 1;
    int row = 0;
    int lane = 0;
    std::pair<double, double> position;
    MapNodeType type = MapNodeType::NormalBattle;
    MapNodeRole role = MapNodeRole::Normal;
    std::vector<MapNode*> next;
    std::vector<int> nextNodeIds;
    bool fixed = false;
};

namespace std {
template <>
struct hash<MapNodeType> {
    size_t operator()(MapNodeType type) const noexcept {
        return static_cast<size_t>(type);
    }
};
}

#endif // WORLD_MAP_MAPNODE_H
