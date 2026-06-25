#ifndef GUI_PAGES_EVENTPAGE_H
#define GUI_PAGES_EVENTPAGE_H

#include "app/gamemanager.h"
#include "combat/equipment/equipment.h"
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
    void recruitTemplateSelected(int templateId);
    void eventEquipmentSelected(EquipmentGroup group, int equipmentId);
    void saveRequested(int slot);
    void bagRequested();
    void shopRequested();
    void equipmentRequested();
    void returnToStartRequested();

private:
    void paintEvent(QPaintEvent* event) override;
    void rebuildOptions();
    QWidget* createHexTechCard(int choiceIndex);
    QWidget* createRestOptionCard(RestOption option);
    QWidget* createRestTrainingCard(int ownedCardIndex);
    QWidget* createEventOwnedCard(int ownedCardIndex);
    QWidget* createRecruitChoiceCard(int templateId);
    QWidget* createEquipmentChoiceCard(EquipmentGroup group, int equipmentId);
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
    QWidget* restOptionsWidget;
    QScrollArea* restScrollArea;
    QGridLayout* restLayout;
    QLabel* restDescriptionLabel;
    QLabel* restEquipmentHintLabel;
    QPushButton* confirmHexTechButton;
    GameState currentState;
    QPixmap backgroundPixmap;
    int selectedHexTechChoice;
};

#endif // GUI_PAGES_EVENTPAGE_H
