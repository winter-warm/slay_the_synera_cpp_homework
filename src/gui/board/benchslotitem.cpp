#include "benchslotitem.h"
#include <QGraphicsSceneHoverEvent>
#include <QPainter>

BenchSlotItem::BenchSlotItem(int row, int col, const QRectF& rect, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , rowId(row)
    , colId(col)
    , rectShape(rect)
    , hoverActive(false)
    , dropActive(false)
    , pointerHover(false) {
    setAcceptHoverEvents(true);
}

QRectF BenchSlotItem::boundingRect() const {
    return rectShape.adjusted(-2.0, -2.0, 2.0, 2.0);
}

void BenchSlotItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    QColor fill(45, 48, 54);
    QColor border(92, 96, 104);

    if (dropActive) {
        fill = QColor(80, 120, 90);
        border = QColor(110, 220, 130);
    } else if (hoverActive || pointerHover) {
        fill = QColor(58, 62, 70);
        border = QColor(150, 155, 165);
    }

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(border, 2));
    painter->setBrush(fill);
    painter->drawRoundedRect(rectShape, 6, 6);
}

QPoint BenchSlotItem::slotPos() const {
    return QPoint(colId, rowId);
}

void BenchSlotItem::setHoverActive(bool active) {
    if (hoverActive == active) {
        return;
    }
    hoverActive = active;
    update();
}

void BenchSlotItem::setDropActive(bool active) {
    if (dropActive == active) {
        return;
    }
    dropActive = active;
    update();
}

void BenchSlotItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    Q_UNUSED(event);
    if (!pointerHover) {
        pointerHover = true;
        update();
    }
}

void BenchSlotItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    Q_UNUSED(event);
    if (pointerHover) {
        pointerHover = false;
        update();
    }
}
