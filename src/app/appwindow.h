#ifndef APP_APPWINDOW_H
#define APP_APPWINDOW_H

#include <QMainWindow>

class BattlePage;
class EventPage;
class GameManager;
class MapPage;
class QStackedWidget;
class StartPage;

class AppWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AppWindow(QWidget* parent = nullptr);

private:
    GameManager* manager;
    QStackedWidget* stack;
    StartPage* startPage;
    MapPage* mapPage;
    EventPage* eventPage;
    BattlePage* battlePage;
};

#endif // APP_APPWINDOW_H
