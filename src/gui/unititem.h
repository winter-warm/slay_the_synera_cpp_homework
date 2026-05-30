#ifndef GUI_ITEMS_UNITITEM_H
#define GUI_ITEMS_UNITITEM_H

#include "core/hex.h"

#include <QGraphicsObject>
#include <QPointF>
#include <memory>

class CharacterHUD;
class QGraphicsSceneMouseEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class Unit;
class QWidget;

class UnitItem : public QGraphicsObject {
    Q_OBJECT

public:
    explicit UnitItem(Unit* unit, QGraphicsItem* parent = nullptr);
    ~UnitItem() override;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    Unit* unit() const { return unitPtr; }
    int unitId() const;

    void setHex(const Hex& h);
    void clearHex();
    bool hasHex() const { return hasCell; }
    Hex hex() const { return cell; }

signals:
    void dragStarted(int unitId, const QPointF& scenePos);
    void dragMoved(int unitId, const QPointF& scenePos);
    void dragDropped(int unitId, const QPointF& scenePos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    Unit* unitPtr;
    Hex cell;
    bool hasCell;
    bool dragging;
    std::unique_ptr<CharacterHUD> hud;
};

#endif // GUI_ITEMS_UNITITEM_H
