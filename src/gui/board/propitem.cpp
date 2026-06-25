#include "gui/board/propitem.h"

#include "entity/component/spritecomponent.h"
#include "entity/object/object.h"
#include "entity/obstacle/obstacle.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

PropItem::PropItem(Object* object, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , objectPtr(object)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setTransformOriginPoint(0.0, 10.0);
    reloadImage();
}

PropItem::PropItem(Obstacle* obstacle, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , obstaclePtr(obstacle)
{
    setAcceptedMouseButtons(Qt::NoButton);
    reloadImage();
}

QRectF PropItem::boundingRect() const
{
    return spriteRect().adjusted(-4.0, -10.0, 4.0, 4.0);
}

void PropItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    const SpriteComponent* propSprite = sprite();
    if (!painter || !propSprite || !propSprite->isVisible()) {
        return;
    }

    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QRectF target = spriteRect();
    if (!pixmap.isNull()) {
        const QPixmap scaled = pixmap.scaled(propSprite->textureWidth(),
                                             propSprite->textureHeight(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
        painter->drawPixmap(QPointF(target.center().x() - scaled.width() * 0.5,
                                    target.center().y() - scaled.height() * 0.5),
                            scaled);
        return;
    }

    if (obstaclePtr) {
        painter->setPen(QPen(obstaclePtr->blocksAttack() ? QColor(54, 64, 62) : QColor(86, 72, 34), 2.0));
        painter->setBrush(obstaclePtr->blocksAttack() ? QColor(92, 126, 96) : QColor(168, 137, 58));
        painter->drawRect(target);
        return;
    }

    painter->setPen(QPen(QColor(90, 56, 16), 2.0));
    painter->setBrush(QColor(196, 126, 36));
    painter->drawRoundedRect(QRectF(-28.0, -24.0, 56.0, 42.0), 6.0, 6.0);
}

int PropItem::propId() const
{
    if (objectPtr) {
        return objectPtr->id();
    }
    return obstaclePtr ? obstaclePtr->id() : -1;
}

void PropItem::setHex(const Hex& h)
{
    cell = h;
}

void PropItem::reloadImage()
{
    pixmap = QPixmap();
    const SpriteComponent* propSprite = sprite();
    if (!propSprite) {
        return;
    }

    const bool alternate = objectPtr && objectPtr->opened();
    const QString relativePath = QString::fromStdString(propSprite->imagePath(alternate));
    const QString runtimePath = QDir(QCoreApplication::applicationDirPath()).filePath(relativePath);
    const QString sourcePath = QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath("../../../" + relativePath);
    for (const QString& path : {runtimePath, sourcePath, relativePath}) {
        if (QFile::exists(path) && pixmap.load(path)) {
            return;
        }
    }
}

void PropItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (!objectPtr || event->button() != Qt::LeftButton) {
        QGraphicsObject::mousePressEvent(event);
        return;
    }
    if (!objectPtr->interact().tryInteract()) {
        event->accept();
        return;
    }

    objectPtr->open();
    reloadImage();
    update();
    playOpenBounce();
    emit opened(objectPtr->id());
    event->accept();
}

void PropItem::playOpenBounce()
{
    auto* group = new QSequentialAnimationGroup(this);
    auto* grow = new QPropertyAnimation(this, "scale", group);
    grow->setDuration(120);
    grow->setStartValue(1.0);
    grow->setEndValue(1.18);

    auto* settle = new QPropertyAnimation(this, "scale", group);
    settle->setDuration(180);
    settle->setStartValue(1.18);
    settle->setEndValue(1.0);

    group->addAnimation(grow);
    group->addAnimation(settle);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

const SpriteComponent* PropItem::sprite() const
{
    if (objectPtr) {
        return &objectPtr->sprite();
    }
    if (obstaclePtr) {
        return &obstaclePtr->sprite();
    }
    return nullptr;
}

QRectF PropItem::spriteRect() const
{
    const SpriteComponent* propSprite = sprite();
    if (!propSprite) {
        return QRectF();
    }
    return QRectF(propSprite->textureOffsetX(),
                  propSprite->textureOffsetY(),
                  propSprite->textureWidth(),
                  propSprite->textureHeight());
}
