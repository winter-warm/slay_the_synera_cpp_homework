#include "gamestate.h"
#include <QJsonArray>
#include <vector>

static QJsonArray intsToJson(const std::vector<int>& values) {
    QJsonArray array;
    for (int value : values) {
        array.append(value);
    }
    return array;
}

static std::vector<int> intsFromJson(const QJsonArray& array) {
    std::vector<int> values;
    for (const QJsonValue& value : array) {
        values.push_back(value.toInt());
    }
    return values;
}

QJsonObject GameState::toJson() const {
    QJsonObject object;
    object["seed"] = seed;
    object["playerHp"] = playerHp;
    object["maxPlayerHp"] = maxPlayerHp;
    object["gold"] = gold;
    object["currentLayerId"] = currentLayerId;
    object["playerNodeId"] = playerNodeId;
    object["currentNodeId"] = currentNodeId;
    object["completedNodeIds"] = intsToJson(completedNodeIds);
    object["availableNodeIds"] = intsToJson(availableNodeIds);
    object["map"] = map.toJson();
    return object;
}

GameState GameState::fromJson(const QJsonObject& object) {
    GameState state;
    state.seed = object.value("seed").toInt();
    state.playerHp = object.value("playerHp").toInt(80);
    state.maxPlayerHp = object.value("maxPlayerHp").toInt(80);
    state.gold = object.value("gold").toInt();
    state.currentLayerId = object.value("currentLayerId").toInt(1);
    state.playerNodeId = object.value("playerNodeId").toInt(-1);
    state.currentNodeId = object.value("currentNodeId").toInt(-1);
    state.completedNodeIds = intsFromJson(object.value("completedNodeIds").toArray());
    state.availableNodeIds = intsFromJson(object.value("availableNodeIds").toArray());
    state.map = MapGraph::fromJson(object.value("map").toObject());
    return state;
}
