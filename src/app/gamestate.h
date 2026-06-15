#ifndef APP_GAMESTATE_H
#define APP_GAMESTATE_H

#include "world/map/mapgraph.h"
#include "world/event/eventtypes.h"
#include "world/bag/bag.h"
#include "world/hextech/hextech.h"
#include <QJsonObject>
#include <string>
#include <vector>

struct CurrentEventState {
    bool active = false;
    int eventId = -1;
    std::string stepId = "start";
    std::string title;
    std::string backgroundPath;
    std::string text;
    std::vector<EventOption> options;
    bool hexTechSelection = false;
};

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
    CurrentEventState currentEvent;
    BattleConfig currentBattle;
    std::string pendingEventStepAfterBattle;
    std::vector<HexTechCardRecord> selectedHexTechCards;
    std::vector<std::string> activeAuraIds;
    std::vector<HexTechDefinition> currentHexTechChoices;
    std::vector<OwnedCharacterCard> ownedCharacterCards;
    ShopState shop;

    QJsonObject toJson() const;
    static GameState fromJson(const QJsonObject& object);
};

#endif // APP_GAMESTATE_H
