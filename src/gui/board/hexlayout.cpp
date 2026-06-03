#include "hexlayout.h"

#include <QtMath>

HexLayout::HexLayout() = default;

void HexLayout::setCells(const std::vector<Hex>& cells) {
    cellList = cells;
}

void HexLayout::setOrigin(const QPointF& p) {
    origin = p;
}

void HexLayout::setSize(qreal r, qreal gap) {
    radius = r;
    rowGap = gap;
}

QPointF HexLayout::toWorld(const Hex& h) const {
    const qreal col = h.x, row = h.z;
    const qreal gap = radius * qSqrt(3.0);
    const qreal shift = (h.z % 2 == 0) ? gap * 0.5 : 0.0;
    return origin + QPointF(shift + col * gap, row * rowGap);
}

std::optional<Hex> HexLayout::fromWorld(const QPointF& p) const {
    std::optional<Hex> best;
    qreal bestD = 1e18;

    for (const Hex& h : cellList) {
        const QPointF c = toWorld(h);
        const qreal dx = p.x() - c.x();
        const qreal dy = p.y() - c.y();
        const qreal d = dx * dx + dy * dy;
        if (d < bestD) {
            bestD = d;
            best = h;
        }
    }

    return best;
}

QPolygonF HexLayout::poly(const Hex& h) const {
    const QPointF c = toWorld(h);
    QPolygonF out;
    out.reserve(6);

    for (int i = 0; i < 6; ++i) {
        const qreal deg = 60.0 * i - 90.0;
        const qreal rad = qDegreesToRadians(deg);
        out.append(QPointF(
            c.x() + radius * qCos(rad),
            c.y() + radius * qSin(rad)
        ));
    }

    return out;
}
