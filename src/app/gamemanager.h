#ifndef APP_GAMEMANAGER_H
#define APP_GAMEMANAGER_H

#include "combat/battlesystem.h"
#include "gamestate.h"
#include "recordmanager.h"
#include "savemanager.h"
#include <QObject>
#include <vector>

class QTimer;

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
    void chooseRestOption(RestOption option);
    void chooseRestTrainingCard(int ownedCardIndex);
    void chooseEventOwnedCard(int ownedCardIndex);
    void finishEvent(const EventResult& result = {});
    void finishBattle(const BattleResult& result = {});
    void startPreparedBattle(const std::vector<CharacterPlacement>& placements);
    void grantBattleChestGold(int gold);
    void buyShopOffer(int offerIndex);
    void mergeOwnedCharacterCards(int firstIndex, int secondIndex);
    void pauseElapsedTimer();

signals:
    void stateChanged(const GameState& state);
    void elapsedSecondsChanged(int seconds);
    void showMapRequested();
    void showEventRequested(int nodeId);
    void showBattleRequested(int nodeId);
    void gameCompletedRequested();
    void gameOverRequested();
    void messageRequested(const QString& title, const QString& message);

private:
    void setState(const GameState& state);
    void unlockLayerStart(int layerId);
    void completeCurrentNode();
    void completeNode(int nodeId);
    void finishCurrentEventNode();
    bool executeEventAction(const EventAction& action, int optionIndex, int actionIndex);
    void executeEventActions(const std::vector<EventAction>& actions, int optionIndex);
    void loadEventStep(int eventId, const std::string& stepId);
    void loadHexTechEvent(int layerId, int nodeId);
    void loadRestEvent();
    void finishRestEvent();
    void grantRandomEquipmentFromRest();
    void startBattle(const BattleConfig& config);
    void ensureCardEconomyInitialized();
    void refreshShopOffers();
    void grantShopExperience(int amount);
    void startElapsedTimer();
    void stopElapsedTimer();
    void tickElapsedTime();
    void autoSave();
    bool shouldAutoSave() const;
    void recordRun(const std::string& result);
    std::vector<std::string> currentCharacterNames() const;

    GameState currentState;
    SaveManager saves;
    RecordManager records;
    battlesystem battleSystem;
    QTimer* elapsedTimer;
    int activeBattleNodeId = -1;
    bool terminalRunFinished = false;
};

#endif // APP_GAMEMANAGER_H
