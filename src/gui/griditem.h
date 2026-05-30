#ifndef GUI_ITEMS_GRIDITEM_H
#define GUI_ITEMS_GRIDITEM_H

#include "core/hex.h"

#include <QColor>
#include <QGraphicsObject>
#include <QPolygonF>

class QGraphicsSceneHoverEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class GridItem : public QGraphicsObject {
    Q_OBJECT

public:
    GridItem(const Hex& hex, const QPolygonF& polygon, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    Hex hex() const { return cell; }

    void setBaseColor(const QColor& color);
    void setHoverActive(bool active);
    void setDropActive(bool active);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    Hex cell;
    QPolygonF polyShape;
    QRectF bounds;
    QColor baseColor;
    bool hoverActive;
    bool dropActive;
    bool pointerHover;
};

#endif // GUI_ITEMS_GRIDITEM_H
