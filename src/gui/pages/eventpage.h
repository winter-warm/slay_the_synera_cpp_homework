#ifndef GUI_PAGES_EVENTPAGE_H
#define GUI_PAGES_EVENTPAGE_H

#include "app/gamemanager.h"
#include <QPixmap>
#include <QWidget>

class GameHud;
class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;

class EventPage : public QWidget {
    Q_OBJECT

public:
    explicit EventPage(QWidget* parent = nullptr);

public slots:
    void setState(const GameState& state);
    void showEvent(int nodeId);

signals:
    void eventFinished(const EventResult& result);
    void eventOptionSelected(int optionIndex);
    void hexTechCardSelected(int choiceIndex);
    void saveRequested(int slot);
    void bagRequested();
    void shopRequested();

private:
    void paintEvent(QPaintEvent* event) override;
    void rebuildOptions();
    QWidget* createHexTechCard(int choiceIndex);
    void updateBackground();

    GameHud* hud;
    QLabel* titleLabel;
    QLabel* bodyLabel;
    QVBoxLayout* optionsLayout;
    QHBoxLayout* hexTechLayout;
    QPushButton* confirmHexTechButton;
    GameState currentState;
    QPixmap backgroundPixmap;
    int selectedHexTechChoice;
};

#endif // GUI_PAGES_EVENTPAGE_H
