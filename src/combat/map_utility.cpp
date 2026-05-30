#include "map_utility.h"
#include "entity/obstacle/obstacle.h"

#include <QQueue>
#include <QSet>
#include <QtMath>

static Hex roundHex(const Hex& from, const Hex& to, double t) {
    const double x = from.x + (to.x - from.x) * t;
    const double y = from.y + (to.y - from.y) * t;
    const double z = from.z + (to.z - from.z) * t;

    int rx = qRound(x), ry = qRound(y), rz = qRound(z);
    const double dx = qAbs(rx - x), dy = qAbs(ry - y), dz = qAbs(rz - z);

    if (dx > dy && dx > dz) {
        rx = -ry - rz;
    } else if (dy > dz) {
        ry = -rx - rz;
    } else {
        rz = -rx - ry;
    }

    return Hex{rx, ry, rz};
}

QVector<Hex> line(const Board& board, const Hex& from, const Hex& to) {
    QVector<Hex> out;
    const int n = board.dist(from, to);
    out.reserve(n + 1);

    if (n == 0) {
        out.push_back(from);
        return out;
    }

    for (int i = 0; i <= n; ++i) {
        out.push_back(roundHex(from, to, double(i) / n));
    }
    return out;
}

bool blockedBetween(const Board& board, const Hex& from, const Hex& to) {
    const QVector<Hex> cells = line(board, from, to);
    for (int i = 1; i + 1 < cells.size(); ++i) {
        Obstacle* obstacle = dynamic_cast<Obstacle*>(board.unitAt(cells[i]));
        if (obstacle && obstacle->getBlockAttack()) {
            return true;
        }
    }
    return false;
}

bool pathExists(const Board& board, const Hex& from, const Hex& to) {
    if (!board.has(from) || !board.has(to)) {
        return false;
    }
    if (from == to) {
        return true;
    }

    QQueue<Hex> queue;
    QSet<Hex> seen;
    queue.enqueue(from);
    seen.insert(from);

    while (!queue.isEmpty()) {
        const Hex cur = queue.dequeue();
        for (const Hex& next : board.neighbors(cur)) {
            if (seen.contains(next)) {
                continue;
            }
            seen.insert(next);
            if (next == to) {
                return true;
            }
            if (!board.passable(next)) {
                continue;
            }
            queue.enqueue(next);
        }
    }
    return false;
}
