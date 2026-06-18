#ifndef GUI_ITEMS_OBJECTITEM_H
#define GUI_ITEMS_OBJECTITEM_H

#include "core/hex.h"

#include <QGraphicsObject>
#include <QPixmap>

class Object;
class QGraphicsSceneMouseEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class ObjectItem : public QGraphicsObject {
    Q_OBJECT

public:
    explicit ObjectItem(Object* object, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    Object* object() const { return objectPtr; }
    int objectId() const;
    void setHex(const Hex& h);
    Hex hex() const { return cell; }
    void reloadImage();

signals:
    void opened(int objectId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void playOpenBounce();

    Object* objectPtr;
    Hex cell;
    QPixmap pixmap;
};

#endif // GUI_ITEMS_OBJECTITEM_H
