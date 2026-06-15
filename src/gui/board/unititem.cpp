#include "gui/board/unititem.h"
#include "entity/unit.h"
#include "entity/character/character.h"
#include "gui/HUD/characterhud.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

UnitItem::UnitItem(Unit* unit, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , unitPtr(unit)
    , cell()
    , hasCell(false)
    , dragging(false)
    , hud(nullptr) {
    setAcceptedMouseButtons(Qt::LeftButton);
    if (Character* character = dynamic_cast<Character*>(unitPtr)) {
        hud = std::make_unique<CharacterHUD>(character->name());
        character->getrender().bindHUD(hud.get());
    }
}

UnitItem::~UnitItem() {
    unitPtr = nullptr;
}

QRectF UnitItem::boundingRect() const {
    return QRectF(-42, -42, 84, 84);
}

void UnitItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    if (hud) {
        hud->paint(painter, boundingRect());
    }
}

int UnitItem::unitId() const {
    return unitPtr ? unitPtr->id() : -1;
}

void UnitItem::setHex(const Hex& h) {
    cell = h;
    hasCell = true;
}

void UnitItem::clearHex() {
    hasCell = false;
}

void UnitItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        QGraphicsObject::mousePressEvent(event);
        return;
    }

    dragging = true;
    emit dragStarted(unitId(), event->scenePos());
    event->accept();
}

void UnitItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (!dragging) {
        QGraphicsObject::mouseMoveEvent(event);
        return;
    }

    emit dragMoved(unitId(), event->scenePos());
    event->accept();
}

void UnitItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (!dragging || event->button() != Qt::LeftButton) {
        QGraphicsObject::mouseReleaseEvent(event);
        return;
    }

    dragging = false;
    emit dragDropped(unitId(), event->scenePos());
    event->accept();
}
