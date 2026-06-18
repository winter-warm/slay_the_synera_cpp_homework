#ifndef GUI_WIDGETS_CARDPANEL_H
#define GUI_WIDGETS_CARDPANEL_H

#include "app/gamestate.h"
#include <QFrame>
#include <QPoint>

class QGridLayout;
class QLabel;
class QPushButton;
class QResizeEvent;
class QEvent;

class CardPanel : public QFrame {
    Q_OBJECT

public:
    enum class Mode { None, Shop, Bag, Training };

    explicit CardPanel(QWidget* parent = nullptr);

    Mode mode() const { return currentMode; }
    void setGameState(const GameState& state);
    void showShop();
    void showBag();
    void showTraining();
    void closePanel();
    void toggleShop();
    void toggleBag();
    void relayoutAnimated(bool animated);

signals:
    void buyRequested(int offerIndex);
    void mergeRequested(int firstIndex, int secondIndex);
    void trainRequested(int ownedCardIndex);

private:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void rebuild();
    void rebuildShop();
    void rebuildBag();
    void rebuildTraining();
    QWidget* createCharacterCard(int templateId, int starLevel, int cost, bool sold, int index, bool bagCard, bool trainingCard = false);
    void updateExpFill();
    void playMergeEffect(const QPoint& center);
    void playCardFlyToBag(QWidget* sourceCard, int offerIndex);
    void playTrailParticle(const QPoint& point, int delayMs);
    void playEnhancedMergeEffect(const QPoint& center);
    void playEnhancedCardFlyToBag(QWidget* sourceCard, int offerIndex);
    void playEnhancedTrailParticle(const QPoint& point, int delayMs, int size, const QString& color);
    QRect shownGeometry() const;
    QRect hiddenGeometry() const;
    int shopExpThreshold() const;
    QString shopProbabilityText() const;
    QString shopExperienceTooltip() const;

    GameState currentState;
    Mode currentMode = Mode::None;
    QLabel* titleLabel;
    QLabel* expLabel;
    QFrame* contentBackdrop;
    QFrame* expTrack;
    QFrame* expFill;
    QGridLayout* cardsLayout;
    QPushButton* closeButton;
    QPoint dragStartPosition;
    int dragSourceIndex = -1;
};

#endif // GUI_WIDGETS_CARDPANEL_H
