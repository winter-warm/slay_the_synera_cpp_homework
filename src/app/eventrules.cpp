#include "eventrules.h"
#include <algorithm>

static bool hasTrainableCard(const GameState& state) {
    for (const OwnedCharacterCard& card : state.ownedCharacterCards) {
        if (card.starLevel < 3) {
            return true;
        }
    }
    return false;
}

static bool hasOwnedCardTemplate(const GameState& state, int templateId) {
    for (const OwnedCharacterCard& card : state.ownedCharacterCards) {
        if (card.templateId == templateId) {
            return true;
        }
    }
    return false;
}

static std::string disabledReasonForRequirement(const GameState& state, const EventRequirement& requirement) {
    if (requirement.type == "goldAtLeast" && state.gold < requirement.amount) {
        return "金币不足，需要 " + std::to_string(requirement.amount) + " 金币";
    }
    if (requirement.type == "hpAbove" && state.playerHp <= requirement.amount) {
        return "生命不足，需要生命值高于 " + std::to_string(requirement.amount);
    }
    if (requirement.type == "hpAtLeast" && state.playerHp < requirement.amount) {
        return "生命不足，需要至少 " + std::to_string(requirement.amount) + " 点生命";
    }
    if (requirement.type == "maxHpAtLeast" && state.maxPlayerHp < requirement.amount) {
        return "生命上限不足，需要至少 " + std::to_string(requirement.amount);
    }
    if (requirement.type == "shopLevelAtLeast" && state.shop.level < requirement.amount) {
        return "商店等级不足，需要等级 " + std::to_string(requirement.amount);
    }
    if (requirement.type == "shopExpAtLeast" && state.shop.exp < requirement.amount) {
        return "商店经验不足，需要 " + std::to_string(requirement.amount);
    }
    if (requirement.type == "hasTrainableCard" && !hasTrainableCard(state)) {
        return "没有未到三星的卡";
    }
    if (requirement.type == "hasOwnedCard" && !hasOwnedCardTemplate(state, requirement.templateId)) {
        return "没有需要的卡牌";
    }
    return {};
}

std::string disabledReasonForEventOption(const GameState& state, const EventOption& option) {
    if (option.requiresGold >= 0 && state.gold < option.requiresGold) {
        return "金币不足，需要 " + std::to_string(option.requiresGold) + " 金币";
    }
    if (option.requiresHpAbove >= 0 && state.playerHp <= option.requiresHpAbove) {
        return "生命不足，需要生命值高于 " + std::to_string(option.requiresHpAbove);
    }
    for (const EventRequirement& requirement : option.requirements) {
        const std::string reason = disabledReasonForRequirement(state, requirement);
        if (!reason.empty()) {
            return reason;
        }
    }
    return {};
}

bool eventOptionRequirementsMet(const GameState& state, const EventOption& option) {
    return disabledReasonForEventOption(state, option).empty();
}

bool ownedCardMatchesEventFilter(const OwnedCharacterCard& card, const std::string& filter) {
    if (filter.empty() || filter == "any") {
        return true;
    }
    if (filter == "starBelow3" || filter == "trainable") {
        return card.starLevel < 3;
    }
    if (filter == "star1") {
        return card.starLevel == 1;
    }
    if (filter == "star2") {
        return card.starLevel == 2;
    }
    if (filter == "star3") {
        return card.starLevel == 3;
    }
    return false;
}

std::vector<int> selectableOwnedCardIndexesForEventFilter(const GameState& state, const std::string& filter) {
    std::vector<int> indexes;
    for (int i = 0; i < static_cast<int>(state.ownedCharacterCards.size()); ++i) {
        if (ownedCardMatchesEventFilter(state.ownedCharacterCards[static_cast<size_t>(i)], filter)) {
            indexes.push_back(i);
        }
    }
    return indexes;
}
