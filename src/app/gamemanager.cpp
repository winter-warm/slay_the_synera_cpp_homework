#include "gamemanager.h"
#include "world/event/eventrepository.h"
#include "world/hextech/hextechrepository.h"
#include "world/map/mapgenerator.h"
#include <algorithm>
#include <random>

static bool containsInt(const std::vector<int>& values, int value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

static std::string hexTechIntroTitleForLayer(int layerId) {
    if (layerId == 1) {
        return "启航";
    }
    if (layerId == 2) {
        return "探索";
    }
    if (layerId == 3) {
        return "决战";
    }
    return "命运的岔口";
}

static std::string hexTechIntroTextForLayer(int layerId) {
    if (layerId == 1) {
        return "你站在晨雾尚未散尽的港口，掌心还残留着昨夜冷汗的盐味。身后的旧世界正缓缓沉入潮声，前方的航路却像一柄未出鞘的剑，在日光里泛着危险的亮。你知道自己不只是要活下去，你要带着这支队伍穿过混乱、饥饿与背叛，把失落的名字重新刻回命运之上。风抬起披风的一角，你向前一步，听见远处钟声震动海面。现在，选择你的海克斯奖励。";
    }
    if (layerId == 2) {
        return "林地深处没有真正的寂静，只有看不见的目光在枝叶间游移。你拨开潮湿的藤蔓，靴底碾过碎石与旧骨，胸口的心跳却比脚步更稳。你已经不再是刚离岸时那个只凭勇气前进的人，胜利、损失、犹疑和愤怒都在你身上沉淀成锋利的判断。前方的幽光一闪而逝，像是在邀请，也像是在试探。你握紧武器，命令队伍继续前进。现在，选择你的海克斯奖励。";
    }
    if (layerId == 3) {
        return "天穹被风暴压低，赤色云层像燃烧的铁幕横在世界尽头。你走在最前方，听见身后每一次呼吸都沉重而真实；他们相信你，而你也不能再把恐惧藏得太深。所有退路都已在来时崩塌，所有牺牲都在此刻索要答案。你抬头望向终战的方向，指节因用力而发白，却没有停下。若命运要审判你，就让它先看见你的刀锋。现在，选择你的海克斯奖励。";
    }
    return "道路在你脚下分开，沉默的力量等待回应。现在，选择你的海克斯奖励。";
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
    emit messageRequested("Saved", "Game saved.");
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

    currentState.currentEvent = {};
    currentState.currentHexTechChoices.clear();
    currentState.pendingEventStepAfterBattle.clear();

    EventContext context;
    context.layerId = currentState.currentLayerId;
    context.nodeId = nodeId;
    context.eventId = node->event_id;
    context.seed = currentState.seed;

    EventRepository repository;
    if (node->event_id == MapEventId::Start) {
        loadHexTechEvent(currentState.currentLayerId, nodeId);
        emit stateChanged(currentState);
        emit showEventRequested(nodeId);
        return;
    }

    if (node->event_id == MapEventId::NormalBattle) {
        if (const auto battle = repository.battleFor(context, BattleKind::Normal)) {
            startBattle(*battle);
            return;
        }
    }
    if (node->event_id == MapEventId::EliteBattle) {
        if (const auto battle = repository.battleFor(context, BattleKind::Elite)) {
            startBattle(*battle);
            return;
        }
    }
    if (node->event_id == MapEventId::Boss) {
        if (const auto battle = repository.battleFor(context, BattleKind::Boss)) {
            startBattle(*battle);
            return;
        }
    }

    loadEventStep(node->event_id, "start");
    emit showEventRequested(nodeId);
}

void GameManager::chooseHexTechCard(int choiceIndex) {
    if (!currentState.currentEvent.active ||
        !currentState.currentEvent.hexTechSelection ||
        choiceIndex < 0 ||
        choiceIndex >= static_cast<int>(currentState.currentHexTechChoices.size())) {
        return;
    }

    const HexTechDefinition definition =
        currentState.currentHexTechChoices[static_cast<size_t>(choiceIndex)];

    HexTechCardRecord record;
    record.hexTechId = definition.id;
    record.layerId = definition.layerId;
    record.title = definition.title;
    record.description = definition.description;
    record.cardImagePath = definition.cardImagePath;
    record.pickedAtNodeId = currentState.currentNodeId;
    currentState.selectedHexTechCards.push_back(std::move(record));

    bool addedLongTerm = false;
    int goldGained = 0;
    bool deferredInterface = false;
    for (const HexTechEffect& effect : definition.effects) {
        if (effect.type == "grant_gold") {
            currentState.gold += effect.value;
            goldGained += effect.value;
        } else if (isLongTermHexTechEffect(effect)) {
            addedLongTerm = true;
        } else {
            deferredInterface = true;
        }
    }

    if (addedLongTerm &&
        std::find(currentState.activeAuraIds.begin(),
                  currentState.activeAuraIds.end(),
                  definition.id) == currentState.activeAuraIds.end()) {
        currentState.activeAuraIds.push_back(definition.id);
    }

    currentState.currentEvent = {};
    currentState.currentHexTechChoices.clear();
    currentState.pendingEventStepAfterBattle.clear();
    completeCurrentNode();
    emit stateChanged(currentState);
    emit showMapRequested();

    if (goldGained > 0) {
        emit messageRequested("HexTech Acquired",
                              QString("Selected %1 and gained %2 gold.")
                                  .arg(QString::fromStdString(definition.title))
                                  .arg(goldGained));
    } else if (deferredInterface) {
        emit messageRequested("HexTech Acquired",
                              QString("%1 was recorded. Its follow-up selection interface is reserved for a later UI.")
                                  .arg(QString::fromStdString(definition.title)));
    }
}

void GameManager::chooseEventOption(int optionIndex) {
    if (!currentState.currentEvent.active ||
        optionIndex < 0 ||
        optionIndex >= static_cast<int>(currentState.currentEvent.options.size())) {
        return;
    }

    const EventOption option = currentState.currentEvent.options[static_cast<size_t>(optionIndex)];
    for (const EventAction& action : option.actions) {
        switch (action.type) {
        case EventActionType::FinishNode:
            currentState.currentEvent = {};
            currentState.pendingEventStepAfterBattle.clear();
            completeCurrentNode();
            emit stateChanged(currentState);
            emit showMapRequested();
            return;
        case EventActionType::GoToStep:
            loadEventStep(currentState.currentEvent.eventId, action.nextStepId);
            emit stateChanged(currentState);
            emit showEventRequested(currentState.currentNodeId);
            return;
        case EventActionType::StartBattle:
            currentState.pendingEventStepAfterBattle = action.battle.returnStepId;
            startBattle(action.battle);
            return;
        }
    }
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
        if (!currentState.pendingEventStepAfterBattle.empty()) {
            const std::string nextStep = currentState.pendingEventStepAfterBattle;
            currentState.pendingEventStepAfterBattle.clear();
            loadEventStep(currentState.currentEvent.eventId, nextStep);
            emit stateChanged(currentState);
            emit showEventRequested(currentState.currentNodeId);
            return;
        }
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

void GameManager::loadEventStep(int eventId, const std::string& stepId) {
    EventContext context;
    context.layerId = currentState.currentLayerId;
    context.nodeId = currentState.currentNodeId;
    context.eventId = eventId;
    context.seed = currentState.seed;

    EventRepository repository;
    const auto event = repository.eventFor(context, stepId);
    if (!event) {
        currentState.currentEvent = {};
        return;
    }

    currentState.currentEvent.active = true;
    currentState.currentEvent.eventId = event->id;
    currentState.currentEvent.stepId = event->stepId;
    currentState.currentEvent.title = event->title;
    currentState.currentEvent.backgroundPath = event->backgroundPath;
    currentState.currentEvent.text = event->text;
    currentState.currentEvent.options = event->options;
    currentState.currentEvent.hexTechSelection = false;
}

void GameManager::loadHexTechEvent(int layerId, int nodeId) {
    HexTechRepository repository;
    currentState.currentHexTechChoices = repository.choicesFor(currentState.seed, layerId, nodeId);

    currentState.currentEvent = {};
    currentState.currentEvent.active = true;
    currentState.currentEvent.eventId = MapEventId::Start;
    currentState.currentEvent.stepId = "start";
    currentState.currentEvent.title = hexTechIntroTitleForLayer(layerId);
    currentState.currentEvent.backgroundPath =
        "assets/events/hextech_start_layer" + std::to_string(layerId) + ".png";
    currentState.currentEvent.text = hexTechIntroTextForLayer(layerId);
    currentState.currentEvent.hexTechSelection = true;
}

void GameManager::startBattle(const BattleConfig& config) {
    currentState.currentBattle = config;
    emit stateChanged(currentState);
    emit showBattleRequested(currentState.currentNodeId);
}
