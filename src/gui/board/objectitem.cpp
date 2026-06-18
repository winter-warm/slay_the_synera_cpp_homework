#include "gui/board/objectitem.h"
#include "entity/object/object.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

ObjectItem::ObjectItem(Object* object, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , objectPtr(object)
    , cell()
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setTransformOriginPoint(0.0, 10.0);
    reloadImage();
}

QRectF ObjectItem::boundingRect() const
{
    return QRectF(-42.0, -52.0, 84.0, 84.0);
}

void ObjectItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    if (!painter || !objectPtr || !objectPtr->render().isVisible()) {
        return;
    }

    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    if (!pixmap.isNull()) {
        const QPixmap scaled = pixmap.scaled(76, 76, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter->drawPixmap(QPointF(-scaled.width() * 0.5, -scaled.height() * 0.55), scaled);
        return;
    }

    painter->setPen(QPen(QColor(90, 56, 16), 2.0));
    painter->setBrush(QColor(196, 126, 36));
    painter->drawRoundedRect(QRectF(-28.0, -24.0, 56.0, 42.0), 6.0, 6.0);
}

int ObjectItem::objectId() const
{
    return objectPtr ? objectPtr->id() : -1;
}

void ObjectItem::setHex(const Hex& h)
{
    cell = h;
}

void ObjectItem::reloadImage()
{
    pixmap = QPixmap();
    if (!objectPtr) {
        return;
    }

    const QString relativePath = QString::fromStdString(objectPtr->render().imagePath(objectPtr->opened()));
    const QString runtimePath = QDir(QCoreApplication::applicationDirPath()).filePath(relativePath);
    const QString sourcePath = QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath("../../../" + relativePath);
    for (const QString& path : {runtimePath, sourcePath, relativePath}) {
        if (QFile::exists(path) && pixmap.load(path)) {
            return;
        }
    }
}

void ObjectItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
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

void ObjectItem::playOpenBounce()
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
