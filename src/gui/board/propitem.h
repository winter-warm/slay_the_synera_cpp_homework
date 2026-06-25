#ifndef GUI_BOARD_PROPITEM_H
#define GUI_BOARD_PROPITEM_H

#include "core/hex.h"

#include <QGraphicsObject>
#include <QPixmap>

class Object;
class Obstacle;
class QGraphicsSceneMouseEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class PropItem : public QGraphicsObject {
    Q_OBJECT

public:
    explicit PropItem(Object* object, QGraphicsItem* parent = nullptr);
    explicit PropItem(Obstacle* obstacle, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    Object* object() const { return objectPtr; }
    Obstacle* obstacle() const { return obstaclePtr; }
    int propId() const;
    void setHex(const Hex& h);
    Hex hex() const { return cell; }
    void reloadImage();

signals:
    void opened(int objectId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void playOpenBounce();
    const class SpriteComponent* sprite() const;
    QRectF spriteRect() const;

    Object* objectPtr = nullptr;
    Obstacle* obstaclePtr = nullptr;
    Hex cell;
    QPixmap pixmap;
};

#endif // GUI_BOARD_PROPITEM_H
