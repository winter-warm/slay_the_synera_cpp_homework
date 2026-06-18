#ifndef APP_EVENTRULES_H
#define APP_EVENTRULES_H

#include "gamestate.h"
#include <string>
#include <vector>

std::string disabledReasonForEventOption(const GameState& state, const EventOption& option);
bool eventOptionRequirementsMet(const GameState& state, const EventOption& option);
bool ownedCardMatchesEventFilter(const OwnedCharacterCard& card, const std::string& filter);
std::vector<int> selectableOwnedCardIndexesForEventFilter(const GameState& state, const std::string& filter);

#endif // APP_EVENTRULES_H
