#ifndef GUI_PAGES_EVENTPAGE_H
#define GUI_PAGES_EVENTPAGE_H

#include "app/gamemanager.h"
#include <QPixmap>
#include <QString>
#include <QWidget>

class GameHud;
class QLabel;
class QHBoxLayout;
class QGridLayout;
class QVBoxLayout;
class QPushButton;
class QScrollArea;
class QWidget;
class NormalEventView;

class EventPage : public QWidget {
    Q_OBJECT

public:
    explicit EventPage(QWidget* parent = nullptr);

public slots:
    void setState(const GameState& state);
    void setElapsedSeconds(int seconds);
    void showEvent(int nodeId);

signals:
    void eventFinished(const EventResult& result);
    void eventOptionSelected(int optionIndex);
    void hexTechCardSelected(int choiceIndex);
    void restOptionSelected(RestOption option);
    void restTrainingCardSelected(int ownedCardIndex);
    void eventOwnedCardSelected(int ownedCardIndex);
    void trainingPanelRequested();
    void saveRequested(int slot);
    void bagRequested();
    void shopRequested();
    void returnToStartRequested();

private:
    void paintEvent(QPaintEvent* event) override;
    void rebuildOptions();
    QWidget* createHexTechCard(int choiceIndex);
    QWidget* createRestOptionCard(RestOption option);
    QWidget* createRestTrainingCard(int ownedCardIndex);
    QWidget* createEventOwnedCard(int ownedCardIndex);
    void setSpecialEventLayoutVisible(bool visible);
    bool hasTrainableCards() const;
    void updateBackground();

    GameHud* hud;
    QVBoxLayout* contentLayout;
    NormalEventView* normalEventView;
    QLabel* titleLabel;
    QLabel* bodyLabel;
    QWidget* optionsWidget;
    QVBoxLayout* optionsLayout;
    QHBoxLayout* hexTechLayout;
    QScrollArea* restScrollArea;
    QGridLayout* restLayout;
    QLabel* restDescriptionLabel;
    QPushButton* confirmHexTechButton;
    GameState currentState;
    QPixmap backgroundPixmap;
    int selectedHexTechChoice;
};

#endif // GUI_PAGES_EVENTPAGE_H
