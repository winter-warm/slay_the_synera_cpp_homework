#ifndef WORLD_EVENT_EVENTTYPES_H
#define WORLD_EVENT_EVENTTYPES_H

#include "core/hex.h"
#include <string>
#include <vector>

enum class BattleKind {
    None,
    Normal,
    Elite,
    Boss
};

enum class EventActionType {
    FinishNode,
    StartBattle,
    GoToStep,
    ShowMessage,
    LotteryScratch,
    Heal,
    LoseHp,
    ChangeMaxHp,
    GrantGold,
    LoseGold,
    GrantRandomGold,
    GrantCard,
    GrantRandomCard,
    UpgradeRandomOwnedCard,
    UpgradeSelectedOwnedCard,
    GrantShopExp,
    GrantRandomEquipment,
    ChooseOwnedCard,
    ChooseRecruit,
    ChooseBasicEquipment,
    ChooseAdvancedEquipment
};

enum class RestOption {
    Rest,
    Train,
    Dig,
    Forge
};

struct EnemyPlacement {
    int templateId = 0;
    Hex hex;
};

struct BattleConfig {
    BattleKind kind = BattleKind::None;
    std::string boardId = "default_board";
    std::vector<EnemyPlacement> enemies;
    float statMultiplier = 1.0f;
    int poolLayerId = 0;
    std::string returnStepId;
    std::string defeatStepId;
};

struct LotteryPayout {
    int value = 0;
    int weight = 1;
};

struct EventAction {
    EventActionType type = EventActionType::FinishNode;
    std::string nextStepId;
    std::string title;
    std::string message;
    std::string winText;
    std::string loseText;
    std::string evenText;
    std::string resultSuffix;
    std::string prompt;
    std::string filter;
    int amount = 0;
    int minAmount = 0;
    int maxAmount = 0;
    int templateId = 0;
    BattleConfig battle;
    std::vector<LotteryPayout> lotteryPayouts;
    std::vector<EventAction> onChooseActions;
};

struct EventRequirement {
    std::string type;
    std::string value;
    int amount = 0;
    int templateId = 0;
};

struct EventOption {
    std::string label;
    std::string description;
    int requiresGold = -1;
    int requiresHpAbove = -1;
    std::vector<EventRequirement> requirements;
    std::vector<EventAction> actions;
};

struct EventDefinition {
    int id = -1;
    std::string stepId = "start";
    std::string title;
    std::string backgroundPath;
    std::string text;
    std::vector<EventOption> options;
};

struct EventContext {
    int layerId = 1;
    int nodeId = -1;
    int eventId = -1;
    int seed = 0;
};

#endif // WORLD_EVENT_EVENTTYPES_H
