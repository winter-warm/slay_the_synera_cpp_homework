#ifndef GUI_PAGES_EVENTPAGE_H
#define GUI_PAGES_EVENTPAGE_H

#include "app/gamemanager.h"
#include <QWidget>

class GameHud;
class QLabel;

class EventPage : public QWidget {
    Q_OBJECT

public:
    explicit EventPage(QWidget* parent = nullptr);

public slots:
    void setState(const GameState& state);
    void showEvent(int nodeId);

signals:
    void eventFinished(const EventResult& result);
    void saveRequested(int slot);

private:
    GameHud* hud;
    QLabel* titleLabel;
};

#endif // GUI_PAGES_EVENTPAGE_H
