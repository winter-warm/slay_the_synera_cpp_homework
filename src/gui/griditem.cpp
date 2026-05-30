#include "griditem.h"
#include <QGraphicsSceneHoverEvent>
#include <QPainter>

GridItem::GridItem(const Hex& hex, const QPolygonF& polygon, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , cell(hex)
    , polyShape(polygon)
    , bounds(polygon.boundingRect())
    , baseColor(QColor(60, 60, 80))
    , hoverActive(false)
    , dropActive(false)
    , pointerHover(false) {
    setAcceptHoverEvents(true);
}

QRectF GridItem::boundingRect() const {
    return bounds.adjusted(-2.0, -2.0, 2.0, 2.0);
}

void GridItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    QColor fill = baseColor;
    QColor border = QColor(40, 40, 40);

    if (dropActive) {
        fill = QColor(110, 170, 110);
        border = QColor(100, 255, 100);
    } else if (hoverActive || pointerHover) {
        fill = baseColor.lighter(120);
    }

    painter->setPen(QPen(border, 2));
    painter->setBrush(fill);
    painter->drawPolygon(polyShape);

    if (hoverActive || pointerHover) {
        painter->setPen(QPen(QColor(220, 220, 220), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawPolygon(polyShape);
    }
}

void GridItem::setBaseColor(const QColor& color) {
    baseColor = color;
    update();
}

void GridItem::setHoverActive(bool active) {
    if (hoverActive == active) {
        return;
    }
    hoverActive = active;
    update();
}

void GridItem::setDropActive(bool active) {
    if (dropActive == active) {
        return;
    }
    dropActive = active;
    update();
}

void GridItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    Q_UNUSED(event);
    if (!pointerHover) {
        pointerHover = true;
        update();
    }
}

void GridItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    Q_UNUSED(event);
    if (pointerHover) {
        pointerHover = false;
        update();
    }
}
