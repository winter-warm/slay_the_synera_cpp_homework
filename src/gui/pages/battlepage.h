#ifndef GUI_PAGES_BATTLEPAGE_H
#define GUI_PAGES_BATTLEPAGE_H

#include "app/gamemanager.h"
#include "combat/battlesystem.h"
#include <QWidget>
#include <memory>
#include <vector>

class Character;
class battlesystem;
class BattleScene;
class GameHud;
class QEvent;
class QGraphicsView;
class QPushButton;
class QTimer;

class BattlePage : public QWidget {
    Q_OBJECT

public:
    explicit BattlePage(QWidget* parent = nullptr);
    ~BattlePage();

    void setBattleSystem(battlesystem* system);
    std::vector<CharacterPlacement> preparedPlacements() const;

public slots:
    void setState(const GameState& state);
    void startNodeBattle(int nodeId);

signals:
    void startBattleRequested();
    void battleFinished(const BattleResult& result);
    void saveRequested(int slot);
    void bagRequested();
    void shopRequested();
    void panelsShouldClose();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void fitSceneInView();
    void ensureStarterRoster();
    void syncRosterFromState();
    void applyStarScaling(Character* character, int starLevel);
    void syncFromBattleSystem();
    void positionSettlementButton();
    void beginSettlement(bool victory);
    void completeSettlement();

    GameState currentState;
    GameHud* hud;
    QGraphicsView* view;
    QPushButton* startButton;
    QPushButton* settlementButton;
    BattleScene* game;
    battlesystem* battleSystem;
    QTimer* battleTimer;
    BattleResult pendingBattleResult;
    bool settlementActive;
    std::vector<std::unique_ptr<Character>> roster;
    std::vector<OwnedCharacterCard> rosterSignature;
};

#endif // GUI_PAGES_BATTLEPAGE_H

