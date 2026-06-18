#ifndef GUI_PAGES_MAPPAGE_H
#define GUI_PAGES_MAPPAGE_H

#include "app/gamestate.h"
#include <QWidget>

class GameHud;
class QGraphicsScene;
class QGraphicsRectItem;
class QGraphicsTextItem;
class QGraphicsView;
class QPropertyAnimation;

class MapPage : public QWidget {
    Q_OBJECT

public:
    explicit MapPage(QWidget* parent = nullptr);

public slots:
    void setState(const GameState& state);
    void setElapsedSeconds(int seconds);

signals:
    void nodeSelected(int nodeId);
    void saveRequested(int slot);
    void bagRequested();
    void shopRequested();
    void returnToStartRequested();

private:
    QString nodeIconPath(const MapNode& node) const;
    void drawLayer(const GameState& state, bool playEntryAnimation);
    void playEntryScroll();

    GameHud* hud;
    QGraphicsView* view;
    QGraphicsScene* scene;
    QPropertyAnimation* scrollAnimation = nullptr;
    QGraphicsRectItem* entryBannerRect = nullptr;
    QGraphicsTextItem* entryBannerText = nullptr;
    int entryAnimationLayerId = -1;
};

#endif // GUI_PAGES_MAPPAGE_H
