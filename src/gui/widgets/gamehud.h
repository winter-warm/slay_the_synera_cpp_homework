#ifndef GUI_WIDGETS_GAMEHUD_H
#define GUI_WIDGETS_GAMEHUD_H

#include "app/gamestate.h"
#include <QWidget>

class QLabel;
class QToolButton;

class GameHud : public QWidget {
    Q_OBJECT

public:
    explicit GameHud(QWidget* parent = nullptr);

public slots:
    void setState(const GameState& state);
    void setElapsedSeconds(int seconds);

signals:
    void saveRequested(int slot);
    void bagRequested();
    void shopRequested();
    void returnToStartRequested();

private:
    void animateGoldGain();

    QLabel* hpIconLabel;
    QLabel* hpLabel;
    QLabel* coinIconLabel;
    QLabel* goldLabel;
    QLabel* timeLabel;
    QToolButton* exitButton;
    QToolButton* bagButton;
    QToolButton* shopButton;
    int lastGold = -1;
};

#endif // GUI_WIDGETS_GAMEHUD_H
