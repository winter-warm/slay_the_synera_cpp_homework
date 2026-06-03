#include "gamemanager.h"
#include "world/map/mapgenerator.h"
#include <algorithm>
#include <random>

static bool containsInt(const std::vector<int>& values, int value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

GameManager::GameManager(QObject* parent)
    : QObject(parent) {}

std::vector<int> GameManager::existingSaveSlots() const {
    return saves.existingSlots();
}

void GameManager::newGame(int seed) {
    if (seed == 0) {
        std::random_device device;
        seed = static_cast<int>(device());
    }

    GameState state;
    state.seed = seed;
    state.map = MapGenerator().generate(seed);
    state.currentLayerId = 1;
    currentState = state;
    for (MapLayer& layer : currentState.map.layers) {
        layer.rebuildNextPointers();
    }
    unlockLayerStart(1);
    emit stateChanged(currentState);
    emit showMapRequested();
}

void GameManager::loadFromSlot(int slot) {
    GameState state;
    std::string error;
    if (!saves.loadGame(&state, slot, &error)) {
        emit messageRequested("Load failed", QString::fromStdString(error));
        return;
    }
    setState(state);
    emit showMapRequested();
}

void GameManager::saveToSlot(int slot) {
    std::string error;
    if (!saves.saveGame(currentState, slot, &error)) {
        emit messageRequested("Save failed", QString::fromStdString(error));
        return;
    }
    emit messageRequested("Saved", QString("Game saved to slot %1.").arg(slot));
}

void GameManager::enterMapNode(int nodeId) {
    if (!containsInt(currentState.availableNodeIds, nodeId)) {
        emit messageRequested("Node locked", "This node is not available yet.");
        return;
    }

    currentState.currentNodeId = nodeId;

    const MapLayer* layer = currentState.map.layerById(currentState.currentLayerId);
    const MapNode* node = layer ? layer->nodeById(nodeId) : nullptr;
    if (!node) {
        return;
    }

    emit showEventRequested(nodeId);
}

void GameManager::finishEvent(const EventResult& result) {
    if (result.startedBattle) {
        emit showBattleRequested(currentState.currentNodeId);
        return;
    }
    completeCurrentNode();
    emit stateChanged(currentState);
    emit showMapRequested();
}

void GameManager::finishBattle(const BattleResult& result) {
    if (result.victory) {
        completeCurrentNode();
        emit stateChanged(currentState);
    }
    emit showMapRequested();
}

void GameManager::setState(const GameState& state) {
    currentState = state;
    for (MapLayer& layer : currentState.map.layers) {
        layer.rebuildNextPointers();
    }
    if (currentState.playerNodeId < 0) {
        const MapLayer* layer = currentState.map.layerById(currentState.currentLayerId);
        if (layer) {
            currentState.playerNodeId = layer->startNodeId;
        }
    }
    emit stateChanged(currentState);
}

void GameManager::unlockLayerStart(int layerId) {
    currentState.currentLayerId = layerId;
    currentState.availableNodeIds.clear();
    const MapLayer* layer = currentState.map.layerById(layerId);
    if (layer) {
        currentState.playerNodeId = layer->startNodeId;
        currentState.currentNodeId = -1;
        currentState.availableNodeIds = {layer->startNodeId};
    }
}

void GameManager::completeCurrentNode() {
    const int nodeId = currentState.currentNodeId;
    if (nodeId < 0) {
        return;
    }

    if (!containsInt(currentState.completedNodeIds, nodeId)) {
        currentState.completedNodeIds.push_back(nodeId);
    }
    currentState.playerNodeId = nodeId;

    const MapLayer* layer = currentState.map.layerById(currentState.currentLayerId);
    const MapNode* node = layer ? layer->nodeById(nodeId) : nullptr;
    if (!layer || !node) {
        return;
    }

    if (node->role == MapNodeRole::End) {
        if (currentState.currentLayerId >= 3) {
            currentState.availableNodeIds.clear();
            emit gameCompletedRequested();
            return;
        }
        unlockLayerStart(currentState.currentLayerId + 1);
        return;
    }

    currentState.availableNodeIds = node->nextNodeIds;
    currentState.currentNodeId = -1;
}
