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
    bool restSelection = false;
    bool restTrainingSelection = false;
    bool ownedCardSelection = false;
    bool recruitSelection = false;
    bool equipmentSelection = false;
    bool advancedEquipmentSelection = false;
    int recruitMinRarity = 1;
    int recruitMaxRarity = 3;
    std::string selectionPrompt;
    std::string selectionFilter;
    std::vector<EventAction> selectionActions;
    int selectedOwnedCardIndex = -1;
};

struct GameState {
    int seed = 0;
    int playerHp = 80;
    int maxPlayerHp = 80;
    int gold = 0;
    int elapsedSeconds = 0;
    int currentLayerId = 1;
    int playerNodeId = -1;
    int currentNodeId = -1;
    MapGraph map;
    std::vector<int> completedNodeIds;
    std::vector<int> availableNodeIds;
    CurrentEventState currentEvent;
    BattleConfig currentBattle;
    std::string pendingEventStepAfterBattle;
    std::string pendingEventDefeatStepAfterBattle;
    std::vector<HexTechCardRecord> selectedHexTechCards;
    std::vector<std::string> activeAuraIds;
    std::vector<HexTechDefinition> currentHexTechChoices;
    std::vector<OwnedCharacterCard> ownedCharacterCards;
    std::vector<OwnedEquipment> ownedEquipment;
    int nextOwnedCardUid = 1;
    int nextEquipmentInstanceId = 1;
    ShopState shop;

    QJsonObject toJson() const;
    static GameState fromJson(const QJsonObject& object);
};

#endif // APP_GAMESTATE_H
