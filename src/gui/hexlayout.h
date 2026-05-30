#ifndef GUI_HEXLAYOUT_H
#define GUI_HEXLAYOUT_H

#include "core/hex.h"

#include <QPointF>
#include <QPolygonF>
#include <QVector>
#include <optional>

class HexLayout {
public:
    HexLayout();

    void setCells(const QVector<Hex>& cells);
    QPointF toWorld(const Hex& h) const;
    std::optional<Hex> fromWorld(const QPointF& p) const;
    QPolygonF poly(const Hex& h) const;

private:
    QVector<Hex> cellList; qreal radius = 46.0, rowGap = 69.0;
};

#endif // GUI_HEXLAYOUT_H
