#ifndef GUI_WIDGETS_CARDPANEL_H
#define GUI_WIDGETS_CARDPANEL_H

#include "app/gamestate.h"
#include <QFrame>
#include <QSet>

class QGridLayout;
class QLabel;
class QPushButton;

class CardPanel : public QFrame {
    Q_OBJECT

public:
    enum class Mode { None, Shop, Bag };

    explicit CardPanel(QWidget* parent = nullptr);

    Mode mode() const { return currentMode; }
    void setGameState(const GameState& state);
    void showShop();
    void showBag();
    void closePanel();
    void toggleShop();
    void toggleBag();
    void relayoutAnimated(bool animated);

signals:
    void buyRequested(int offerIndex);
    void mergeRequested(int firstIndex, int secondIndex);

private:
    void rebuild();
    void rebuildShop();
    void rebuildBag();
    QWidget* createCharacterCard(int templateId, int starLevel, int cost, bool sold, int index, bool bagCard);
    QRect shownGeometry() const;
    QRect hiddenGeometry() const;
    int shopExpThreshold() const;

    GameState currentState;
    Mode currentMode = Mode::None;
    QLabel* titleLabel;
    QLabel* expLabel;
    QFrame* expFill;
    QGridLayout* cardsLayout;
    QPushButton* closeButton;
    QSet<int> selectedBagIndexes;
};

#endif // GUI_WIDGETS_CARDPANEL_H
