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
    GoToStep
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
    std::string returnStepId;
};

struct EventAction {
    EventActionType type = EventActionType::FinishNode;
    std::string nextStepId;
    BattleConfig battle;
};

struct EventOption {
    std::string label;
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
