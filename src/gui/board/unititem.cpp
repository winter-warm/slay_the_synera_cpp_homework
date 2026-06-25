#include "gui/board/unititem.h"
#include "entity/unit.h"
#include "entity/character/character.h"
#include "gui/HUD/characterhud.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QString>
#include <algorithm>

namespace {
void drawStatusBar(QPainter* painter,
                   const QRectF& rect,
                   qreal ratio,
                   const QColor& fill,
                   const QColor& border)
{
    if (!painter) {
        return;
    }

    ratio = std::clamp<qreal>(ratio, 0.0, 1.0);
    painter->setPen(QPen(border, 1.0));
    painter->setBrush(QColor(18, 18, 18, 170));
    painter->drawRoundedRect(rect, 2.0, 2.0);

    if (ratio <= 0.0) {
        return;
    }

    QRectF fillRect = rect.adjusted(1.0, 1.0, -1.0, -1.0);
    fillRect.setWidth(fillRect.width() * ratio);
    painter->setPen(Qt::NoPen);
    painter->setBrush(fill);
    painter->drawRoundedRect(fillRect, 1.5, 1.5);
}

void drawStatusText(QPainter* painter, const QPointF& pos, const QString& text, const QColor& color)
{
    if (!painter) {
        return;
    }

    QFont font = painter->font();
    font.setPointSize(7);
    font.setBold(true);
    painter->setFont(font);

    QPainterPath path;
    path.addText(pos, font, text);
    painter->setPen(QPen(QColor(15, 12, 10, 220), 2.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawPath(path);
}
}

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
    }
}

UnitItem::~UnitItem() {
    unitPtr = nullptr;
}

QRectF UnitItem::boundingRect() const {
    return QRectF(-42, -42, 152, 104);
}

void UnitItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    Character* character = dynamic_cast<Character*>(unitPtr);
    if (!character || !painter) {
        return;
    }
    if (!character->getrender().inVisible()) {
        return;
    }
    if (hud) {
        hud->setVisible(true);
        hud->paint(painter, QRectF(-42, -42, 84, 84));
    }

    StatsComponent& stats = character->getstats();
    const qreal hpRatio = stats.getMAXHP() > 0
                              ? static_cast<qreal>(stats.getHP()) / static_cast<qreal>(stats.getMAXHP())
                              : 0.0;
    const qreal shieldRatio = stats.getMAXHP() > 0
                                  ? static_cast<qreal>(stats.getSHIELD()) / static_cast<qreal>(stats.getMAXHP())
                                  : 0.0;
    const qreal mpRatio = stats.getMAXMP() > 0
                              ? static_cast<qreal>(stats.getMP()) / static_cast<qreal>(stats.getMAXMP())
                              : 0.0;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    const QRectF hpBar(-30.0, 34.0, 60.0, 5.0);
    const QRectF shieldBar(-30.0, 41.0, 60.0, 5.0);
    const QRectF mpBar(-30.0, 48.0, 60.0, 5.0);
    drawStatusBar(painter, hpBar, hpRatio, QColor(210, 38, 45), QColor(90, 18, 20));
    drawStatusBar(painter, shieldBar, shieldRatio, QColor(165, 165, 170), QColor(82, 82, 88));
    drawStatusBar(painter, mpBar, mpRatio, QColor(48, 112, 220), QColor(20, 55, 120));
    drawStatusText(painter,
                   QPointF(34.0, 40.0),
                   QString::fromUtf8("血:%1/%2").arg(stats.getHP()).arg(stats.getMAXHP()),
                   QColor(255, 225, 225));
    drawStatusText(painter,
                   QPointF(34.0, 47.0),
                   QString::fromUtf8("盾:%1").arg(stats.getSHIELD()),
                   QColor(232, 232, 236));
    drawStatusText(painter,
                   QPointF(34.0, 54.0),
                   QString::fromUtf8("蓝:%1/%2").arg(stats.getMP()).arg(stats.getMAXMP()),
                   QColor(220, 235, 255));
    painter->restore();
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
