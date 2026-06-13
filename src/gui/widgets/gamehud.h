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

signals:
    void saveRequested(int slot);

private:
    QLabel* hpIconLabel;
    QLabel* hpLabel;
    QLabel* coinIconLabel;
    QLabel* goldLabel;
    QToolButton* exitButton;
    QToolButton* bagButton;
    QToolButton* shopButton;
};

#endif // GUI_WIDGETS_GAMEHUD_H
