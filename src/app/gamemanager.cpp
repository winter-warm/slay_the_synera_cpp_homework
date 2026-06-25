#include "gamemanager.h"
#include "combat/battlerepository.h"
#include "combat/equipment/equipmentrepository.h"
#include "entity/character/characterfactory.h"
#include "eventrules.h"
#include "world/event/eventrepository.h"
#include "world/hextech/hextechrepository.h"
#include "world/map/mapgenerator.h"
#include <QTimer>
#include <algorithm>
#include <random>
#include <unordered_set>

static bool containsInt(const std::vector<int>& values, int value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

static constexpr int kShopOfferCount = 5;

static int shopExpThresholdForLevel(int level) {
    if (level == 1) {
        return 10;
    }
    if (level == 2) {
        return 15;
    }
    if (level == 3) {
        return 20;
    }
    return 0;
}

static int shopCostForRarity(int rarity) {
    switch (rarity) {
    case 1:
        return 1;
    case 2:
        return 3;
    case 3:
        return 5;
    case 4:
        return 10;
    default:
        return 1;
    }
}

static int rollRarityForShopLevel(int level, std::mt19937& rng) {
    static const int probabilities[4][4] = {
        {75, 25, 0, 0},
        {50, 30, 20, 0},
        {50, 20, 20, 10},
        {20, 20, 30, 30},
    };
    const int row = std::max(0, std::min(3, level - 1));
    std::uniform_int_distribution<int> distribution(1, 100);
    int roll = distribution(rng);
    int cumulative = 0;
    for (int i = 0; i < 4; ++i) {
        cumulative += probabilities[row][i];
        if (roll <= cumulative) {
            return i + 1;
        }
    }
    return 1;
}

static int battlePenaltyForLayer(int layerId, BattleKind kind) {
    int penalty = 10 + 5 * std::max(1, layerId);
    if (kind == BattleKind::Elite) {
        penalty *= 2;
    }
    return penalty;
}

static BattleNodeContext battleContextFor(const GameState& state, const MapNode& node, BattleKind kind) {
    BattleNodeContext context;
    context.layerId = state.currentLayerId;
    context.nodeId = node.id;
    context.nodeRow = node.row;
    context.seed = state.seed;
    context.kind = kind;
    return context;
}

static std::mt19937 eventActionRng(const GameState& state, int optionIndex, int actionIndex) {
    return std::mt19937(static_cast<unsigned int>(state.seed * 2654435761u +
                                                  state.currentNodeId * 1103515245u +
                                                  state.currentEvent.eventId * 97u +
                                                  optionIndex * 31u +
                                                  actionIndex * 17u));
}

static std::string replaceAll(std::string text, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return text;
    }
    size_t position = 0;
    while ((position = text.find(from, position)) != std::string::npos) {
        text.replace(position, from.size(), to);
        position += to.size();
    }
    return text;
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
    : QObject(parent)
    , battleSystem(this)
    , elapsedTimer(new QTimer(this)) {
    elapsedTimer->setInterval(1000);
    connect(elapsedTimer, &QTimer::timeout, this, &GameManager::tickElapsedTime);
}

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
    state.gold = 5;
    state.currentLayerId = 1;
    state.nextOwnedCardUid = 1;
    state.nextEquipmentInstanceId = 1;
    state.ownedCharacterCards = {{1209, 1, state.nextOwnedCardUid++}};
    state.shop = {};
    currentState = state;
    terminalRunFinished = false;
    for (MapLayer& layer : currentState.map.layers) {
        layer.rebuildNextPointers();
    }
    refreshShopOffers();
    unlockLayerStart(1);
    startElapsedTimer();
    emit stateChanged(currentState);
    emit showMapRequested();
}

static constexpr int kMaxOwnedCharacterCards = 24;

static int maxOwnedCharacterCards() {
    return kMaxOwnedCharacterCards;
}

void GameManager::loadFromSlot(int slot) {
    GameState state;
    std::string error;
    if (!saves.loadGame(&state, slot, &error)) {
        emit messageRequested("Load failed", QString::fromStdString(error));
        return;
    }
    currentState = state;
    terminalRunFinished = false;
    ensureCardEconomyInitialized();
    startElapsedTimer();
    setState(currentState);
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
    currentState.pendingEventDefeatStepAfterBattle.clear();

    EventContext context;
    context.layerId = currentState.currentLayerId;
    context.nodeId = nodeId;
    context.eventId = node->event_id;
    context.seed = currentState.seed;

    EventRepository repository;
    BattleRepository battleRepository;
    if (node->event_id == MapEventId::Start) {
        loadHexTechEvent(currentState.currentLayerId, nodeId);
        emit stateChanged(currentState);
        emit showEventRequested(nodeId);
        return;
    }

    if (node->event_id == MapEventId::Rest) {
        loadRestEvent();
        emit stateChanged(currentState);
        emit showEventRequested(nodeId);
        return;
    }

    if (node->event_id == MapEventId::NormalBattle) {
        if (const auto battle = battleRepository.battleFor(battleContextFor(currentState, *node, BattleKind::Normal))) {
            startBattle(*battle);
            return;
        }
    }
    if (node->event_id == MapEventId::EliteBattle) {
        if (const auto battle = battleRepository.battleFor(battleContextFor(currentState, *node, BattleKind::Elite))) {
            startBattle(*battle);
            return;
        }
    }
    if (node->event_id == MapEventId::Boss) {
        if (const auto battle = battleRepository.battleFor(battleContextFor(currentState, *node, BattleKind::Boss))) {
            startBattle(*battle);
            return;
        }
    }

    loadEventStep(node->event_id, "start");
    emit stateChanged(currentState);
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
    bool chooseBasicEquipment = false;
    bool chooseAdvancedEquipment = false;
    for (const HexTechEffect& effect : definition.effects) {
        if (effect.type == "grant_gold") {
            currentState.gold += effect.value;
            goldGained += effect.value;
        } else if (effect.type == "choose_basic_equipment") {
            chooseBasicEquipment = true;
        } else if (effect.type == "choose_advanced_equipment") {
            chooseAdvancedEquipment = true;
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

    if (chooseBasicEquipment || chooseAdvancedEquipment) {
        currentState.currentHexTechChoices.clear();
        currentState.pendingEventStepAfterBattle.clear();
        currentState.pendingEventDefeatStepAfterBattle.clear();
        currentState.currentEvent = {};
        currentState.currentEvent.active = true;
        currentState.currentEvent.eventId = MapEventId::Start;
        currentState.currentEvent.stepId = "equipmentChoice";
        currentState.currentEvent.title = definition.title;
        currentState.currentEvent.backgroundPath =
            "assets/events/hextech_start_layer" + std::to_string(currentState.currentLayerId) + ".png";
        currentState.currentEvent.text = definition.description;
        currentState.currentEvent.equipmentSelection = true;
        currentState.currentEvent.advancedEquipmentSelection = chooseAdvancedEquipment;
        currentState.currentEvent.selectionPrompt =
            chooseAdvancedEquipment
                ? QString::fromUtf8("选择一件高级装备。").toStdString()
                : QString::fromUtf8("选择一件初级装备。").toStdString();
        emit stateChanged(currentState);
        emit showEventRequested(currentState.currentNodeId);
        return;
    }

    currentState.currentEvent = {};
    currentState.currentHexTechChoices.clear();
    currentState.pendingEventStepAfterBattle.clear();
    currentState.pendingEventDefeatStepAfterBattle.clear();
    finishCurrentEventNode();

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

void GameManager::chooseRestOption(RestOption option) {
    if (!currentState.currentEvent.active || !currentState.currentEvent.restSelection) {
        return;
    }

    switch (option) {
    case RestOption::Rest:
        currentState.playerHp = std::min(currentState.maxPlayerHp, currentState.playerHp + 15);
        finishRestEvent();
        emit messageRequested(QString::fromUtf8("休息"),
                              QString::fromUtf8("恢复了 15 点生命。"));
        return;
    case RestOption::Train:
        for (const OwnedCharacterCard& card : currentState.ownedCharacterCards) {
            if (card.starLevel < 3) {
                currentState.currentEvent.restSelection = false;
                currentState.currentEvent.restTrainingSelection = true;
                currentState.currentEvent.text = QString::fromUtf8("选择一张未到达三星的卡，升一星。").toStdString();
                emit stateChanged(currentState);
                emit showEventRequested(currentState.currentNodeId);
                return;
            }
        }
        emit messageRequested(QString::fromUtf8("锻炼"),
                              QString::fromUtf8("没有可以升星的卡。"));
        return;
    case RestOption::Dig: {
        std::mt19937 rng(static_cast<unsigned int>(currentState.seed * 2654435761u +
                                                   currentState.currentNodeId * 97u + 17u));
        std::uniform_int_distribution<int> distribution(1, 5);
        const int goldGained = distribution(rng);
        currentState.gold += goldGained;
        finishRestEvent();
        emit messageRequested(QString::fromUtf8("挖宝"),
                              QString::fromUtf8("获得了 %1 金币。").arg(goldGained));
        return;
    }
    case RestOption::Forge:
        grantRandomEquipmentFromRest();
        finishRestEvent();
        return;
    }
}

void GameManager::chooseRestTrainingCard(int ownedCardIndex) {
    if (!currentState.currentEvent.active ||
        !currentState.currentEvent.restTrainingSelection ||
        ownedCardIndex < 0 ||
        ownedCardIndex >= static_cast<int>(currentState.ownedCharacterCards.size())) {
        return;
    }

    OwnedCharacterCard& card = currentState.ownedCharacterCards[static_cast<size_t>(ownedCardIndex)];
    if (card.starLevel >= 3) {
        return;
    }
    ++card.starLevel;
    finishRestEvent();
    emit messageRequested(QString::fromUtf8("锻炼"),
                          QString::fromUtf8("卡牌升为 %1 星。").arg(card.starLevel));
}

void GameManager::chooseEventOption(int optionIndex) {
    if (!currentState.currentEvent.active ||
        optionIndex < 0 ||
        optionIndex >= static_cast<int>(currentState.currentEvent.options.size())) {
        return;
    }

    const EventOption option = currentState.currentEvent.options[static_cast<size_t>(optionIndex)];
    const std::string disabledReason = disabledReasonForEventOption(currentState, option);
    if (!disabledReason.empty()) {
        emit messageRequested(QString::fromUtf8("????"), QString::fromStdString(disabledReason));
        return;
    }

    executeEventActions(option.actions, optionIndex);
    emit stateChanged(currentState);
}

void GameManager::chooseEventOwnedCard(int ownedCardIndex) {
    if (!currentState.currentEvent.active ||
        !currentState.currentEvent.ownedCardSelection ||
        ownedCardIndex < 0 ||
        ownedCardIndex >= static_cast<int>(currentState.ownedCharacterCards.size())) {
        return;
    }
    if (!ownedCardMatchesEventFilter(currentState.ownedCharacterCards[static_cast<size_t>(ownedCardIndex)],
                                     currentState.currentEvent.selectionFilter)) {
        return;
    }

    currentState.currentEvent.selectedOwnedCardIndex = ownedCardIndex;
    currentState.currentEvent.ownedCardSelection = false;
    executeEventActions(currentState.currentEvent.selectionActions, 0);
    emit stateChanged(currentState);
}

void GameManager::chooseEventRecruit(int templateId) {
    if (!currentState.currentEvent.active || !currentState.currentEvent.recruitSelection) {
        return;
    }
    const auto info = characterfactory::infoFor(templateId);
    if (!info || !info->playerCharacter ||
        info->rarity < currentState.currentEvent.recruitMinRarity ||
        info->rarity > currentState.currentEvent.recruitMaxRarity) {
        return;
    }

    currentState.ownedCharacterCards.push_back(makeOwnedCharacterCard(templateId, 1));
    grantShopExperience(1);
    finishCurrentEventNode();
}

void GameManager::chooseEventEquipment(EquipmentGroup group, int equipmentId) {
    if (!currentState.currentEvent.active || !currentState.currentEvent.equipmentSelection || equipmentId <= 0) {
        return;
    }

    EquipmentRepository repository;
    std::optional<Equipment> equipment;
    if (currentState.currentEvent.advancedEquipmentSelection) {
        if (group != EquipmentGroup::Advanced) {
            return;
        }
        equipment = repository.findAdvanced(equipmentId);
    } else {
        if (group == EquipmentGroup::Advanced) {
            return;
        }
        equipment = repository.findBasic(group, equipmentId);
    }
    if (!equipment) {
        return;
    }

    currentState.ownedEquipment.push_back(makeOwnedEquipment(group, equipmentId));
    finishCurrentEventNode();
}

void GameManager::executeEventActions(const std::vector<EventAction>& actions, int optionIndex) {
    for (int actionIndex = 0; actionIndex < static_cast<int>(actions.size()); ++actionIndex) {
        if (!executeEventAction(actions[static_cast<size_t>(actionIndex)], optionIndex, actionIndex)) {
            return;
        }
    }
}

bool GameManager::executeEventAction(const EventAction& action, int optionIndex, int actionIndex) {
    switch (action.type) {
    case EventActionType::FinishNode:
        finishCurrentEventNode();
        return false;
    case EventActionType::GoToStep:
        loadEventStep(currentState.currentEvent.eventId, action.nextStepId);
        emit stateChanged(currentState);
        emit showEventRequested(currentState.currentNodeId);
        return false;
    case EventActionType::StartBattle:
        currentState.pendingEventStepAfterBattle = action.battle.returnStepId;
        currentState.pendingEventDefeatStepAfterBattle = action.battle.defeatStepId;
        if (!action.battle.enemies.empty()) {
            startBattle(action.battle);
            return false;
        }
        if (const MapLayer* layer = currentState.map.layerById(currentState.currentLayerId)) {
            if (const MapNode* node = layer->nodeById(currentState.currentNodeId)) {
                BattleRepository battleRepository;
                BattleKind kind = action.battle.kind == BattleKind::None ? BattleKind::Normal : action.battle.kind;
                BattleNodeContext context = battleContextFor(currentState, *node, kind);
                if (action.battle.poolLayerId > 0) {
                    context.layerId = action.battle.poolLayerId;
                }
                if (const auto battle = battleRepository.battleFor(context)) {
                    BattleConfig config = *battle;
                    config.returnStepId = action.battle.returnStepId;
                    config.defeatStepId = action.battle.defeatStepId;
                    config.poolLayerId = action.battle.poolLayerId;
                    startBattle(config);
                }
            }
        }
        return false;
    case EventActionType::ShowMessage: {
        const QString title = action.title.empty()
                                  ? QString::fromStdString(currentState.currentEvent.title)
                                  : QString::fromStdString(action.title);
        emit messageRequested(title, QString::fromStdString(action.message));
        return true;
    }
    case EventActionType::LotteryScratch: {
        if (currentState.gold < action.amount) {
            emit messageRequested(QString::fromUtf8("金币不足"), QString::fromUtf8("你的金币不够。"));
            return false;
        }
        if (action.lotteryPayouts.empty()) {
            return false;
        }

        currentState.gold -= action.amount;
        int totalWeight = 0;
        for (const LotteryPayout& payout : action.lotteryPayouts) {
            totalWeight += std::max(1, payout.weight);
        }

        std::mt19937 rng = eventActionRng(currentState, optionIndex, actionIndex);
        std::uniform_int_distribution<int> distribution(1, std::max(1, totalWeight));
        int roll = distribution(rng);
        LotteryPayout selected = action.lotteryPayouts.front();
        for (const LotteryPayout& payout : action.lotteryPayouts) {
            roll -= std::max(1, payout.weight);
            if (roll <= 0) {
                selected = payout;
                break;
            }
        }
        currentState.gold += std::max(0, selected.value);

        std::string resultText = selected.value > action.amount
                                     ? action.winText
                                     : (selected.value < action.amount ? action.loseText : action.evenText);
        resultText = replaceAll(resultText, "{payout}", std::to_string(selected.value));
        resultText = replaceAll(resultText, "{cost}", std::to_string(action.amount));
        if (!action.resultSuffix.empty()) {
            resultText += "\n\n" + action.resultSuffix;
        }

        currentState.currentEvent.text = resultText;
        currentState.currentEvent.options.clear();
        EventOption leaveOption;
        leaveOption.label = QString::fromUtf8("离开").toStdString();
        leaveOption.description = QString::fromUtf8("继续赶路").toStdString();
        leaveOption.actions.push_back(EventAction{});
        currentState.currentEvent.options.push_back(leaveOption);
        emit stateChanged(currentState);
        emit showEventRequested(currentState.currentNodeId);
        return false;
    }
    case EventActionType::Heal:
        currentState.playerHp = std::min(currentState.maxPlayerHp,
                                         currentState.playerHp + std::max(0, action.amount));
        return true;
    case EventActionType::LoseHp:
        currentState.playerHp = std::max(0, currentState.playerHp - std::max(0, action.amount));
        return true;
    case EventActionType::ChangeMaxHp:
        currentState.maxPlayerHp = std::max(1, currentState.maxPlayerHp + action.amount);
        currentState.playerHp = std::min(currentState.playerHp, currentState.maxPlayerHp);
        return true;
    case EventActionType::GrantGold:
        currentState.gold += std::max(0, action.amount);
        return true;
    case EventActionType::LoseGold:
        currentState.gold = std::max(0, currentState.gold - std::max(0, action.amount));
        return true;
    case EventActionType::GrantRandomGold: {
        int minGold = action.minAmount > 0 ? action.minAmount : 1;
        int maxGold = action.maxAmount > 0 ? action.maxAmount : (action.amount > 0 ? action.amount : minGold);
        if (maxGold < minGold) {
            std::swap(minGold, maxGold);
        }
        std::mt19937 rng = eventActionRng(currentState, optionIndex, actionIndex);
        std::uniform_int_distribution<int> distribution(minGold, maxGold);
        currentState.gold += distribution(rng);
        return true;
    }
    case EventActionType::GrantCard:
        if (action.templateId > 0) {
            currentState.ownedCharacterCards.push_back(makeOwnedCharacterCard(action.templateId, 1));
            grantShopExperience(1);
        }
        return true;
    case EventActionType::GrantRandomCard: {
        const std::vector<characterfactory::CharacterTemplateInfo> templates = characterfactory::playerTemplates();
        if (!templates.empty()) {
            std::mt19937 rng = eventActionRng(currentState, optionIndex, actionIndex);
            std::uniform_int_distribution<int> pick(0, static_cast<int>(templates.size()) - 1);
            const auto& selected = templates[static_cast<size_t>(pick(rng))];
            currentState.ownedCharacterCards.push_back(makeOwnedCharacterCard(selected.templateId, 1));
            grantShopExperience(1);
        }
        return true;
    }
    case EventActionType::UpgradeRandomOwnedCard: {
        const std::vector<int> candidates = selectableOwnedCardIndexesForEventFilter(currentState, "starBelow3");
        if (!candidates.empty()) {
            std::mt19937 rng = eventActionRng(currentState, optionIndex, actionIndex);
            std::uniform_int_distribution<int> pick(0, static_cast<int>(candidates.size()) - 1);
            OwnedCharacterCard& card =
                currentState.ownedCharacterCards[static_cast<size_t>(candidates[static_cast<size_t>(pick(rng))])];
            ++card.starLevel;
        }
        return true;
    }
    case EventActionType::UpgradeSelectedOwnedCard:
        if (currentState.currentEvent.selectedOwnedCardIndex >= 0 &&
            currentState.currentEvent.selectedOwnedCardIndex <
                static_cast<int>(currentState.ownedCharacterCards.size())) {
            OwnedCharacterCard& card =
                currentState.ownedCharacterCards[static_cast<size_t>(currentState.currentEvent.selectedOwnedCardIndex)];
            if (card.starLevel < 3) {
                ++card.starLevel;
            }
        }
        return true;
    case EventActionType::GrantShopExp:
        grantShopExperience(std::max(0, action.amount));
        return true;
    case EventActionType::GrantRandomEquipment:
        grantRandomEquipmentFromRest();
        return true;
    case EventActionType::ChooseOwnedCard:
        if (selectableOwnedCardIndexesForEventFilter(currentState, action.filter).empty()) {
            emit messageRequested(QString::fromUtf8("条件不足"), QString::fromUtf8("没有符合条件的卡牌。"));
            return false;
        }
        currentState.currentEvent.ownedCardSelection = true;
        currentState.currentEvent.selectionPrompt = action.prompt.empty()
                                                       ? QString::fromUtf8("选择一张卡牌").toStdString()
                                                       : action.prompt;
        currentState.currentEvent.selectionFilter = action.filter;
        currentState.currentEvent.selectionActions = action.onChooseActions;
        currentState.currentEvent.selectedOwnedCardIndex = -1;
        emit stateChanged(currentState);
        emit showEventRequested(currentState.currentNodeId);
        return false;
    case EventActionType::ChooseRecruit:
        currentState.currentEvent.recruitSelection = true;
        currentState.currentEvent.recruitMinRarity = action.minAmount > 0 ? action.minAmount : 1;
        currentState.currentEvent.recruitMaxRarity = action.maxAmount > 0 ? action.maxAmount : 3;
        if (currentState.currentEvent.recruitMaxRarity < currentState.currentEvent.recruitMinRarity) {
            std::swap(currentState.currentEvent.recruitMaxRarity, currentState.currentEvent.recruitMinRarity);
        }
        currentState.currentEvent.selectionPrompt = action.prompt.empty()
                                                       ? QString::fromUtf8("选择一个角色加入。").toStdString()
                                                       : action.prompt;
        currentState.currentEvent.options.clear();
        emit stateChanged(currentState);
        emit showEventRequested(currentState.currentNodeId);
        return false;
    case EventActionType::ChooseBasicEquipment:
    case EventActionType::ChooseAdvancedEquipment:
        currentState.currentEvent.equipmentSelection = true;
        currentState.currentEvent.advancedEquipmentSelection = action.type == EventActionType::ChooseAdvancedEquipment;
        currentState.currentEvent.selectionPrompt = action.prompt.empty()
                                                       ? (currentState.currentEvent.advancedEquipmentSelection
                                                              ? QString::fromUtf8("选择一件高级装备。").toStdString()
                                                              : QString::fromUtf8("选择一件初级装备。").toStdString())
                                                       : action.prompt;
        currentState.currentEvent.options.clear();
        emit stateChanged(currentState);
        emit showEventRequested(currentState.currentNodeId);
        return false;
    }
    return true;
}

void GameManager::finishEvent(const EventResult& result) {
    if (result.startedBattle) {
        emit showBattleRequested(currentState.currentNodeId);
        return;
    }
    finishCurrentEventNode();
}

void GameManager::finishBattle(const BattleResult& result) {
    const int completedBattleNodeId =
        activeBattleNodeId >= 0 ? activeBattleNodeId : currentState.currentNodeId;
    battleSystem.clear();
    if (result.victory) {
        grantShopExperience(1);
    } else {
        currentState.playerHp = std::max(0,
                                         currentState.playerHp -
                                             battlePenaltyForLayer(currentState.currentLayerId,
                                                                   currentState.currentBattle.kind));
        if (currentState.playerHp <= 0) {
            recordRun("death");
            stopElapsedTimer();
            std::string error;
            if (!saves.deleteSave(1, &error)) {
                emit messageRequested("Delete save failed", QString::fromStdString(error));
            }
            emit stateChanged(currentState);
            activeBattleNodeId = -1;
            terminalRunFinished = true;
            emit gameOverRequested();
            return;
        }
    }
    if (!currentState.pendingEventStepAfterBattle.empty() ||
        !currentState.pendingEventDefeatStepAfterBattle.empty()) {
        std::string nextStep = currentState.pendingEventStepAfterBattle;
        if (!result.victory && !currentState.pendingEventDefeatStepAfterBattle.empty()) {
            nextStep = currentState.pendingEventDefeatStepAfterBattle;
        }
        currentState.pendingEventStepAfterBattle.clear();
        currentState.pendingEventDefeatStepAfterBattle.clear();
        loadEventStep(currentState.currentEvent.eventId, nextStep);
        emit stateChanged(currentState);
        emit showEventRequested(currentState.currentNodeId);
        activeBattleNodeId = -1;
        return;
    }
    completeNode(completedBattleNodeId);
    if (terminalRunFinished) {
        emit stateChanged(currentState);
        activeBattleNodeId = -1;
        return;
    }
    if (shouldAutoSave()) {
        autoSave();
    }
    emit stateChanged(currentState);
    activeBattleNodeId = -1;
    emit showMapRequested();
}

void GameManager::startPreparedBattle(const std::vector<CharacterPlacement>& placements) {
    battleSystem.startFromPreparedUnits(placements);
}

void GameManager::grantBattleChestGold(int gold) {
    if (gold <= 0) {
        return;
    }
    currentState.gold += gold;
    emit stateChanged(currentState);
}

void GameManager::grantBattleChestEquipment(EquipmentGroup group, int equipmentId) {
    if (equipmentId <= 0) {
        return;
    }
    currentState.ownedEquipment.push_back(makeOwnedEquipment(group, equipmentId));
    emit stateChanged(currentState);
}

void GameManager::unequipAllEquipment() {
    if (!isRestEventActive()) {
        return;
    }
    for (OwnedEquipment& equipment : currentState.ownedEquipment) {
        equipment.equippedCardUid = -1;
    }
    emit stateChanged(currentState);
}

void GameManager::composeEquipment(int firstInstanceId, int secondInstanceId) {
    if (!isRestEventActive() || firstInstanceId == secondInstanceId) {
        return;
    }

    OwnedEquipment* first = findOwnedEquipment(firstInstanceId);
    OwnedEquipment* second = findOwnedEquipment(secondInstanceId);
    if (!first || !second || first->equippedCardUid >= 0 || second->equippedCardUid >= 0) {
        return;
    }

    EquipmentRepository repository;
    const auto firstEquipment = first->group == EquipmentGroup::Advanced
                                    ? repository.findAdvanced(first->equipmentId)
                                    : repository.findBasic(first->group, first->equipmentId);
    const auto secondEquipment = second->group == EquipmentGroup::Advanced
                                     ? repository.findAdvanced(second->equipmentId)
                                     : repository.findBasic(second->group, second->equipmentId);
    if (!firstEquipment || !secondEquipment) {
        return;
    }

    const auto result = repository.composeResult(*firstEquipment, *secondEquipment);
    if (!result) {
        return;
    }

    currentState.ownedEquipment.erase(
        std::remove_if(currentState.ownedEquipment.begin(), currentState.ownedEquipment.end(),
                       [firstInstanceId, secondInstanceId](const OwnedEquipment& equipment) {
                           return equipment.instanceId == firstInstanceId ||
                                  equipment.instanceId == secondInstanceId;
                       }),
        currentState.ownedEquipment.end());
    currentState.ownedEquipment.push_back(makeOwnedEquipment(EquipmentGroup::Advanced, result->id()));
    emit stateChanged(currentState);
}

void GameManager::equipEquipmentToCard(int equipmentInstanceId, int ownedCardUid) {
    OwnedEquipment* equipment = findOwnedEquipment(equipmentInstanceId);
    if (!equipment || equipment->equippedCardUid >= 0 || ownedCardUid <= 0) {
        return;
    }

    const bool cardExists =
        std::any_of(currentState.ownedCharacterCards.begin(), currentState.ownedCharacterCards.end(),
                    [ownedCardUid](const OwnedCharacterCard& card) {
                        return card.uid == ownedCardUid;
                    });
    if (!cardExists) {
        return;
    }
    const bool cardAlreadyEquipped =
        std::any_of(currentState.ownedEquipment.begin(), currentState.ownedEquipment.end(),
                    [ownedCardUid](const OwnedEquipment& item) {
                        return item.equippedCardUid == ownedCardUid;
                    });
    if (cardAlreadyEquipped) {
        return;
    }

    equipment->equippedCardUid = ownedCardUid;
    emit stateChanged(currentState);
}

void GameManager::buyShopOffer(int offerIndex) {
    ensureCardEconomyInitialized();
    if (offerIndex < 0 || offerIndex >= static_cast<int>(currentState.shop.offers.size())) {
        return;
    }

    if (static_cast<int>(currentState.ownedCharacterCards.size()) >= maxOwnedCharacterCards()) {
        emit messageRequested(QString::fromUtf8("背包满了"),
                              QString::fromUtf8("最多持有 %1 个角色。").arg(maxOwnedCharacterCards()));
        return;
    }

    ShopOffer& offer = currentState.shop.offers[static_cast<size_t>(offerIndex)];
    if (offer.sold || offer.templateId <= 0) {
        return;
    }

    const auto info = characterfactory::infoFor(offer.templateId);
    if (!info) {
        return;
    }
    const int cost = shopCostForRarity(info->rarity);
    if (currentState.gold < cost) {
        emit messageRequested(QString::fromUtf8("金币不足"),
                              QString::fromUtf8("这张卡需要 %1 金币。").arg(cost));
        return;
    }

    currentState.gold -= cost;
    currentState.ownedCharacterCards.push_back(makeOwnedCharacterCard(offer.templateId, 1));
    offer.sold = true;
    grantShopExperience(1);
    emit stateChanged(currentState);
}

void GameManager::recycleOwnedCharacterCard(int ownedCardUid) {
    auto it = std::find_if(currentState.ownedCharacterCards.begin(), currentState.ownedCharacterCards.end(),
                           [ownedCardUid](const OwnedCharacterCard& card) {
                               return card.uid == ownedCardUid;
                           });
    if (it == currentState.ownedCharacterCards.end()) {
        return;
    }
    if (currentState.ownedCharacterCards.size() <= 1) {
        emit messageRequested(QString::fromUtf8("无法回收"),
                              QString::fromUtf8("至少保留一个角色。"));
        return;
    }

    const auto info = characterfactory::infoFor(it->templateId);
    const int cost = info ? shopCostForRarity(info->rarity) : 1;
    const int refund = std::max(0, (it->starLevel - 1) * cost - 1);
    const int uid = it->uid;
    for (OwnedEquipment& equipment : currentState.ownedEquipment) {
        if (equipment.equippedCardUid == uid) {
            equipment.equippedCardUid = -1;
        }
    }
    currentState.ownedCharacterCards.erase(it);
    currentState.gold += refund;
    emit stateChanged(currentState);
    emit centerToastRequested(QString::fromUtf8("回收获得 %1 金币").arg(refund));
}

void GameManager::mergeOwnedCharacterCards(int firstIndex, int secondIndex) {
    ensureCardEconomyInitialized();
    if (firstIndex == secondIndex ||
        firstIndex < 0 ||
        secondIndex < 0 ||
        firstIndex >= static_cast<int>(currentState.ownedCharacterCards.size()) ||
        secondIndex >= static_cast<int>(currentState.ownedCharacterCards.size())) {
        return;
    }

    if (firstIndex > secondIndex) {
        std::swap(firstIndex, secondIndex);
    }

    const OwnedCharacterCard first = currentState.ownedCharacterCards[static_cast<size_t>(firstIndex)];
    const OwnedCharacterCard second = currentState.ownedCharacterCards[static_cast<size_t>(secondIndex)];
    if (first.templateId != second.templateId || first.starLevel != second.starLevel || first.starLevel >= 3) {
        return;
    }

    const int firstUid = first.uid;
    const int secondUid = second.uid;
    currentState.ownedCharacterCards.erase(currentState.ownedCharacterCards.begin() + secondIndex);
    currentState.ownedCharacterCards.erase(currentState.ownedCharacterCards.begin() + firstIndex);
    OwnedCharacterCard merged = makeOwnedCharacterCard(first.templateId, first.starLevel + 1);
    for (OwnedEquipment& equipment : currentState.ownedEquipment) {
        if (equipment.equippedCardUid == firstUid || equipment.equippedCardUid == secondUid) {
            equipment.equippedCardUid = -1;
        }
    }
    currentState.ownedCharacterCards.push_back(merged);
    emit stateChanged(currentState);
    emit centerToastRequested(QString::fromUtf8("合成成功，(如有)装备已自动脱下"));
}

void GameManager::pauseElapsedTimer() {
    stopElapsedTimer();
}

void GameManager::setState(const GameState& state) {
    currentState = state;
    ensureCardEconomyInitialized();
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
    completeNode(currentState.currentNodeId);
}

void GameManager::finishCurrentEventNode() {
    currentState.currentEvent = {};
    currentState.currentHexTechChoices.clear();
    currentState.pendingEventStepAfterBattle.clear();
    currentState.pendingEventDefeatStepAfterBattle.clear();
    completeCurrentNode();
    if (terminalRunFinished) {
        emit stateChanged(currentState);
        return;
    }
    if (shouldAutoSave()) {
        autoSave();
    }
    emit stateChanged(currentState);
    emit showMapRequested();
}

void GameManager::completeNode(int nodeId) {
    if (nodeId < 0) {
        return;
    }

    currentState.currentNodeId = nodeId;
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
            recordRun("clear");
            stopElapsedTimer();
            terminalRunFinished = true;
            std::string error;
            if (!saves.deleteSave(1, &error)) {
                emit messageRequested("Delete save failed", QString::fromStdString(error));
            }
            emit gameCompletedRequested();
            return;
        }
        unlockLayerStart(currentState.currentLayerId + 1);
        refreshShopOffers();
        return;
    }

    currentState.availableNodeIds = node->nextNodeIds;
    currentState.currentNodeId = -1;
    refreshShopOffers();
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
    currentState.currentEvent.restSelection = false;
    currentState.currentEvent.restTrainingSelection = false;
    currentState.currentEvent.ownedCardSelection = false;
    currentState.currentEvent.recruitSelection = false;
    currentState.currentEvent.equipmentSelection = false;
    currentState.currentEvent.advancedEquipmentSelection = false;
    currentState.currentEvent.recruitMinRarity = 1;
    currentState.currentEvent.recruitMaxRarity = 3;
    currentState.currentEvent.selectionPrompt.clear();
    currentState.currentEvent.selectionFilter.clear();
    currentState.currentEvent.selectionActions.clear();
    currentState.currentEvent.selectedOwnedCardIndex = -1;
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

void GameManager::loadRestEvent() {
    currentState.currentEvent = {};
    currentState.currentEvent.active = true;
    currentState.currentEvent.eventId = MapEventId::Rest;
    currentState.currentEvent.stepId = "start";
    currentState.currentEvent.title = QString::fromUtf8("火堆").toStdString();
    currentState.currentEvent.backgroundPath = "assets/events/rest_campfire.png";
    currentState.currentEvent.text = QString::fromUtf8("火光驱散黑暗，短暂的喘息摆在眼前。").toStdString();
    currentState.currentEvent.restSelection = true;
}

void GameManager::finishRestEvent() {
    finishCurrentEventNode();
}

void GameManager::grantRandomEquipmentFromRest() {
    EquipmentRepository repository;
    struct EquipmentKey {
        EquipmentGroup group = EquipmentGroup::Mold;
        int id = 0;
        std::string title;
    };
    std::vector<EquipmentKey> pool;
    for (const Equipment& equipment : repository.allBasic()) {
        pool.push_back({equipment.group(), equipment.id(), equipment.title()});
    }
    if (pool.empty()) {
        return;
    }

    std::mt19937 rng(static_cast<unsigned int>(currentState.seed * 2654435761u +
                                               currentState.currentNodeId * 1103515245u +
                                               currentState.ownedEquipment.size() * 97u + 41u));
    std::uniform_int_distribution<int> pick(0, static_cast<int>(pool.size()) - 1);
    const EquipmentKey selected = pool[static_cast<size_t>(pick(rng))];
    currentState.ownedEquipment.push_back(makeOwnedEquipment(selected.group, selected.id));
    emit messageRequested(QString::fromUtf8("锻造"),
                          QString::fromUtf8("获得装备：%1").arg(QString::fromStdString(selected.title)));
}

OwnedCharacterCard GameManager::makeOwnedCharacterCard(int templateId, int starLevel) {
    OwnedCharacterCard card;
    card.templateId = templateId;
    card.starLevel = std::max(1, std::min(3, starLevel));
    card.uid = currentState.nextOwnedCardUid++;
    return card;
}

OwnedEquipment GameManager::makeOwnedEquipment(EquipmentGroup group, int equipmentId) {
    OwnedEquipment equipment;
    equipment.instanceId = currentState.nextEquipmentInstanceId++;
    equipment.group = group;
    equipment.equipmentId = equipmentId;
    equipment.equippedCardUid = -1;
    return equipment;
}

OwnedEquipment* GameManager::findOwnedEquipment(int instanceId) {
    for (OwnedEquipment& equipment : currentState.ownedEquipment) {
        if (equipment.instanceId == instanceId) {
            return &equipment;
        }
    }
    return nullptr;
}

const OwnedEquipment* GameManager::findOwnedEquipment(int instanceId) const {
    for (const OwnedEquipment& equipment : currentState.ownedEquipment) {
        if (equipment.instanceId == instanceId) {
            return &equipment;
        }
    }
    return nullptr;
}

bool GameManager::isRestEventActive() const {
    return currentState.currentEvent.active &&
           currentState.currentEvent.eventId == MapEventId::Rest;
}

void GameManager::startBattle(const BattleConfig& config) {
    currentState.currentBattle = config;
    activeBattleNodeId = currentState.currentNodeId;
    battleSystem.setActiveAuraIds(currentState.activeAuraIds);
    battleSystem.loadBattle(config);
    emit stateChanged(currentState);
    emit showBattleRequested(currentState.currentNodeId);
}

void GameManager::ensureCardEconomyInitialized() {
    if (currentState.ownedCharacterCards.empty()) {
        currentState.ownedCharacterCards.push_back(makeOwnedCharacterCard(1209, 1));
    }
    if (currentState.shop.level < 1) {
        currentState.shop.level = 1;
    }
    if (currentState.shop.level > 4) {
        currentState.shop.level = 4;
    }
    if (currentState.shop.exp < 0) {
        currentState.shop.exp = 0;
    }
    if (currentState.shop.offers.size() != kShopOfferCount) {
        refreshShopOffers();
    }
}

void GameManager::refreshShopOffers() {
    currentState.shop.offers.clear();
    const std::vector<characterfactory::CharacterTemplateInfo> templates = characterfactory::playerTemplates();
    if (templates.empty()) {
        return;
    }

    std::mt19937 rng(static_cast<unsigned int>(currentState.seed * 1103515245u +
                                               currentState.shop.rollCounter * 2654435761u +
                                               currentState.shop.level * 97u));
    for (int slot = 0; slot < kShopOfferCount; ++slot) {
        const int rarity = rollRarityForShopLevel(currentState.shop.level, rng);
        std::vector<int> candidates;
        for (const auto& info : templates) {
            if (info.rarity == rarity) {
                candidates.push_back(info.templateId);
            }
        }
        if (candidates.empty()) {
            for (const auto& info : templates) {
                candidates.push_back(info.templateId);
            }
        }
        std::uniform_int_distribution<int> pick(0, static_cast<int>(candidates.size()) - 1);
        currentState.shop.offers.push_back({candidates[static_cast<size_t>(pick(rng))], false});
    }
    ++currentState.shop.rollCounter;
}

void GameManager::grantShopExperience(int amount) {
    if (amount <= 0) {
        return;
    }
    currentState.shop.exp += amount;
    while (currentState.shop.level < 4) {
        const int threshold = shopExpThresholdForLevel(currentState.shop.level);
        if (threshold <= 0 || currentState.shop.exp < threshold) {
            break;
        }
        currentState.shop.exp -= threshold;
        ++currentState.shop.level;
    }
    if (currentState.shop.level >= 4) {
        currentState.shop.level = 4;
        currentState.shop.exp = std::max(0, currentState.shop.exp);
    }
}

int GameManager::maxOwnedCharacterCards() const {
    return ::maxOwnedCharacterCards();
}

void GameManager::startElapsedTimer() {
    if (!elapsedTimer->isActive()) {
        elapsedTimer->start();
    }
}

void GameManager::stopElapsedTimer() {
    if (elapsedTimer->isActive()) {
        elapsedTimer->stop();
    }
}

void GameManager::tickElapsedTime() {
    ++currentState.elapsedSeconds;
    emit elapsedSecondsChanged(currentState.elapsedSeconds);
}

void GameManager::autoSave() {
    std::string error;
    if (!saves.saveGame(currentState, 1, &error)) {
        emit messageRequested("Autosave failed", QString::fromStdString(error));
    }
}

bool GameManager::shouldAutoSave() const {
    if (currentState.playerHp <= 0) {
        return false;
    }
    return !(currentState.currentLayerId >= 3 && currentState.availableNodeIds.empty());
}

void GameManager::recordRun(const std::string& result) {
    RunRecord record;
    record.result = result;
    record.reachedLayer = currentState.currentLayerId;
    record.characterNames = currentCharacterNames();
    record.elapsedSeconds = currentState.elapsedSeconds;

    std::string error;
    if (!records.appendRecord(record, &error)) {
        emit messageRequested("Record failed", QString::fromStdString(error));
    }
}

std::vector<std::string> GameManager::currentCharacterNames() const {
    std::vector<std::string> names;
    std::unordered_set<int> seen;
    for (const OwnedCharacterCard& card : currentState.ownedCharacterCards) {
        if (card.templateId <= 0 || seen.find(card.templateId) != seen.end()) {
            continue;
        }
        seen.insert(card.templateId);
        const auto info = characterfactory::infoFor(card.templateId);
        if (info && !info->displayName.empty()) {
            names.push_back(info->displayName);
        } else if (info) {
            names.push_back(info->name);
        }
    }
    return names;
}
