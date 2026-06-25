#ifndef GUI_PAGES_BATTLEPAGE_H
#define GUI_PAGES_BATTLEPAGE_H

#include "app/gamemanager.h"
#include "combat/battlesystem.h"
#include "gui/battle/battlescene.h"
#include <QWidget>
#include <memory>
#include <unordered_map>
#include <vector>

class Character;
class battlesystem;
class GameHud;
class QEvent;
class QGraphicsView;
class QLabel;
class QPushButton;
class QTimer;
class QWidget;

class BattlePage : public QWidget {
    Q_OBJECT

public:
    explicit BattlePage(QWidget* parent = nullptr);
    ~BattlePage();

    void setBattleSystem(battlesystem* system);
    std::vector<CharacterPlacement> preparedPlacements() const;

public slots:
    void setState(const GameState& state);
    void setElapsedSeconds(int seconds);
    void startNodeBattle(int nodeId);

signals:
    void startBattleRequested();
    void battleFinished(const BattleResult& result);
    void battleChestOpened(int gold);
    void battleChestEquipmentClaimed(EquipmentGroup group, int equipmentId);
    void saveRequested(int slot);
    void bagRequested();
    void shopRequested();
    void equipmentRequested();
    void equipEquipmentRequested(int equipmentInstanceId, int ownedCardUid);
    void recycleOwnedCardRequested(int ownedCardUid);
    void battlePreparationModeChanged(bool active);
    void panelsShouldClose();
    void returnToStartRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void fitSceneInView();
    void ensureStarterRoster();
    void syncRosterFromState();
    void applyStarScaling(Character* character, int starLevel);
    int deploymentLimitForCurrentLayer() const;
    void syncFromBattleSystem();
    void ensureBondSummary();
    void clearBondSummary();
    void updateBondSummary();
    void positionSettlementButton();
    void beginSettlement(bool victory);
    void completeSettlement();
    void completeVictoryChest();
    int victoryChestGold() const;
    int rewardModifierPercent(const std::string& modifierKey) const;
    std::vector<BattleScene::EquipmentRewardVisual> rollVictoryEquipmentDrops() const;
    int extraChestGoldFromHexTech() const;
    bool isEquipmentDropAllowed() const;
    void showEquipmentWarning();

    GameState currentState;
    GameHud* hud;
    QGraphicsView* view;
    QPushButton* startButton;
    QPushButton* settlementButton;
    QLabel* equipmentWarningLabel;
    QWidget* zoneLegend;
    QLabel* bondSummaryLabel;
    BattleScene* game;
    battlesystem* battleSystem;
    QTimer* battleTimer;
    BattleResult pendingBattleResult;
    bool settlementActive;
    bool chestSettlementPending;
    int pendingRewardEquipmentClaims = 0;
    std::vector<BattleScene::EquipmentRewardVisual> pendingRewardEquipmentDrops;
    std::vector<std::unique_ptr<Character>> roster;
    std::vector<OwnedCharacterCard> rosterSignature;
    std::unordered_map<int, int> unitIdToOwnedCardUid;
};

#endif // GUI_PAGES_BATTLEPAGE_H

