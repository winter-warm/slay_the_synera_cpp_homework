#ifndef APP_GAMEMANAGER_H
#define APP_GAMEMANAGER_H

#include "combat/battlesystem.h"
#include "gamestate.h"
#include "savemanager.h"
#include <QObject>
#include <vector>

struct EventResult {
    bool startedBattle = false;
};

struct BattleResult {
    bool victory = true;
};

class GameManager : public QObject {
    Q_OBJECT

public:
    explicit GameManager(QObject* parent = nullptr);

    const GameState& state() const { return currentState; }
    battlesystem& currentBattleSystem() { return battleSystem; }
    std::vector<int> existingSaveSlots() const;

public slots:
    void newGame(int seed);
    void loadFromSlot(int slot);
    void saveToSlot(int slot);
    void enterMapNode(int nodeId);
    void chooseEventOption(int optionIndex);
    void chooseHexTechCard(int choiceIndex);
    void finishEvent(const EventResult& result = {});
    void finishBattle(const BattleResult& result = {});
    void startPreparedBattle(const std::vector<CharacterPlacement>& placements);
    void buyShopOffer(int offerIndex);
    void mergeOwnedCharacterCards(int firstIndex, int secondIndex);

signals:
    void stateChanged(const GameState& state);
    void showMapRequested();
    void showEventRequested(int nodeId);
    void showBattleRequested(int nodeId);
    void gameCompletedRequested();
    void messageRequested(const QString& title, const QString& message);

private:
    void setState(const GameState& state);
    void unlockLayerStart(int layerId);
    void completeCurrentNode();
    void completeNode(int nodeId);
    void loadEventStep(int eventId, const std::string& stepId);
    void loadHexTechEvent(int layerId, int nodeId);
    void startBattle(const BattleConfig& config);
    void ensureCardEconomyInitialized();
    void refreshShopOffers();
    void grantShopExperience(int amount);

    GameState currentState;
    SaveManager saves;
    battlesystem battleSystem;
    int activeBattleNodeId = -1;
};

#endif // APP_GAMEMANAGER_H
