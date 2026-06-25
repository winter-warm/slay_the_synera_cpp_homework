#ifndef GUI_WIDGETS_EQUIPMENTPANEL_H
#define GUI_WIDGETS_EQUIPMENTPANEL_H

#include "app/gamestate.h"
#include "combat/equipment/equipmentrepository.h"
#include <QFrame>
#include <optional>

class EquipmentItemWidget;
class QLabel;
class QPushButton;
class QScrollArea;
class QGridLayout;

class EquipmentPanel : public QFrame {
    Q_OBJECT
    friend class EquipmentItemWidget;

public:
    explicit EquipmentPanel(QWidget* parent = nullptr);

    void setGameState(const GameState& state);
    void setBattlePreparationMode(bool enabled);
    void togglePanel();
    void closePanel();
    void relayoutAnimated(bool animate);

    std::optional<Equipment> resolveEquipment(const OwnedEquipment& owned) const;
    bool canDragEquipment(const OwnedEquipment& owned) const;
    bool canComposeEquipment(int sourceInstanceId, int targetInstanceId) const;
    bool isRestActive() const;

signals:
    void unequipAllRequested();
    void composeRequested(int firstInstanceId, int secondInstanceId);

public slots:
    void showDetailFor(EquipmentItemWidget* item);
    void hideDetail();
    void requestCompose(int firstInstanceId, int secondInstanceId);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void rebuild();
    QString richNumberText(const QString& text) const;
    QString recipeTextFor(const Equipment& equipment) const;
    QString groupName(EquipmentGroup group) const;
    OwnedEquipment* ownedByInstanceId(int instanceId);
    const OwnedEquipment* ownedByInstanceId(int instanceId) const;
    void playComposeBurst();

    GameState currentState;
    EquipmentRepository repository;
    bool battlePreparationMode = false;
    QPushButton* unequipButton;
    QScrollArea* scrollArea;
    QWidget* gridHost;
    QGridLayout* gridLayout;
    QFrame* detailPopup;
};

#endif // GUI_WIDGETS_EQUIPMENTPANEL_H
