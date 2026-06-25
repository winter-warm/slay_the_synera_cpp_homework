#ifndef APP_APPWINDOW_H
#define APP_APPWINDOW_H

#include <QMainWindow>

class BattlePage;
class CardPanel;
class EquipmentPanel;
class EventPage;
class GameManager;
class QLabel;
class QResizeEvent;
class MapPage;
class QStackedWidget;
class StartPage;

class AppWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AppWindow(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    BattlePage* ensureBattlePage();
    void showCenterToast(const QString& text);
    void positionCenterToast();

    GameManager* manager;
    QStackedWidget* stack;
    StartPage* startPage;
    MapPage* mapPage;
    EventPage* eventPage;
    BattlePage* battlePage;
    CardPanel* cardPanel;
    EquipmentPanel* equipmentPanel;
    QLabel* centerToastLabel;
};

#endif // APP_APPWINDOW_H
