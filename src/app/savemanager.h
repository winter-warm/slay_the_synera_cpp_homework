#ifndef APP_SAVEMANAGER_H
#define APP_SAVEMANAGER_H

#include "gamestate.h"
#include <string>
#include <vector>

class SaveManager {
public:
    bool saveGame(const GameState& state, int slot, std::string* error = nullptr) const;
    bool loadGame(GameState* state, int slot, std::string* error = nullptr) const;
    bool deleteSave(int slot, std::string* error = nullptr) const;
    std::vector<int> existingSlots() const;

private:
    std::string savePath(int slot) const;
};

#endif // APP_SAVEMANAGER_H
