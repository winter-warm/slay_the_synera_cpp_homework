#ifndef GUI_PAGES_BATTLEPAGE_H
#define GUI_PAGES_BATTLEPAGE_H

#include "app/gamemanager.h"
#include <QWidget>
#include <memory>
#include <vector>

class Character;
class BattleScene;
class GameHud;
class QGraphicsView;
class QPushButton;

class BattlePage : public QWidget {
    Q_OBJECT

public:
    explicit BattlePage(QWidget* parent = nullptr);
    ~BattlePage();

public slots:
    void setState(const GameState& state);
    void startNodeBattle(int nodeId);

signals:
    void battleFinished(const BattleResult& result);
    void saveRequested(int slot);

private:
    void fitSceneInView();
    void ensureStarterRoster();

    GameState currentState;
    GameHud* hud;
    QGraphicsView* view;
    QPushButton* startButton;
    QPushButton* winButton;
    BattleScene* game;
    std::vector<std::unique_ptr<Character>> roster;
};

#endif // GUI_PAGES_BATTLEPAGE_H

