#ifndef GUI_HEXLAYOUT_H
#define GUI_HEXLAYOUT_H

#include "core/hex.h"

#include <QPointF>
#include <QPolygonF>
#include <optional>
#include <vector>

class HexLayout {
public:
    HexLayout();

    void setCells(const std::vector<Hex>& cells);
    void setOrigin(const QPointF& p);
    void setSize(qreal r, qreal gap);
    QPointF toWorld(const Hex& h) const;
    std::optional<Hex> fromWorld(const QPointF& p) const;
    QPolygonF poly(const Hex& h) const;

private:
    std::vector<Hex> cellList; QPointF origin; qreal radius = 46.0, rowGap = 69.0;
};

#endif // GUI_HEXLAYOUT_H
