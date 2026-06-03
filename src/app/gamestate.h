#ifndef APP_GAMESTATE_H
#define APP_GAMESTATE_H

#include "world/map/mapgraph.h"
#include <QJsonObject>
#include <vector>

struct GameState {
    int seed = 0;
    int playerHp = 80;
    int maxPlayerHp = 80;
    int gold = 0;
    int currentLayerId = 1;
    int playerNodeId = -1;
    int currentNodeId = -1;
    MapGraph map;
    std::vector<int> completedNodeIds;
    std::vector<int> availableNodeIds;

    QJsonObject toJson() const;
    static GameState fromJson(const QJsonObject& object);
};

#endif // APP_GAMESTATE_H
