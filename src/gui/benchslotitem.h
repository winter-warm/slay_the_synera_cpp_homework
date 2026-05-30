#ifndef GUI_ITEMS_BENCHSLOTITEM_H
#define GUI_ITEMS_BENCHSLOTITEM_H

#include <QGraphicsObject>
#include <QPoint>
#include <QRectF>

class QGraphicsSceneHoverEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class BenchSlotItem : public QGraphicsObject {
    Q_OBJECT

public:
    BenchSlotItem(int row, int col, const QRectF& rect, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QPoint slotPos() const;
    void setHoverActive(bool active);
    void setDropActive(bool active);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    int rowId;
    int colId;
    QRectF rectShape;
    bool hoverActive;
    bool dropActive;
    bool pointerHover;
};

#endif // GUI_ITEMS_BENCHSLOTITEM_H
