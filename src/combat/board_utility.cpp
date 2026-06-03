#include "board_utility.h"
#include "entity/obstacle/obstacle.h"

#include <cmath>
#include <queue>
#include <unordered_set>

static Hex roundHex(const Hex& from, const Hex& to, double t) {
    const double x = from.x + (to.x - from.x) * t;
    const double y = from.y + (to.y - from.y) * t;
    const double z = from.z + (to.z - from.z) * t;

    int rx = static_cast<int>(std::round(x));
    int ry = static_cast<int>(std::round(y));
    int rz = static_cast<int>(std::round(z));
    const double dx = std::abs(rx - x), dy = std::abs(ry - y), dz = std::abs(rz - z);

    if (dx > dy && dx > dz) {
        rx = -ry - rz;
    } else if (dy > dz) {
        ry = -rx - rz;
    } else {
        rz = -rx - ry;
    }

    return Hex{rx, ry, rz};
}

std::vector<Hex> line(const Board& board, const Hex& from, const Hex& to) {
    std::vector<Hex> out;
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
    const std::vector<Hex> cells = line(board, from, to);
    for (int i = 1; i + 1 < static_cast<int>(cells.size()); ++i) {
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

    std::queue<Hex> queue;
    std::unordered_set<Hex> seen;
    queue.push(from);
    seen.insert(from);

    while (!queue.empty()) {
        const Hex cur = queue.front();
        queue.pop();
        for (const Hex& next : board.neighbors(cur)) {
            if (seen.find(next) != seen.end()) {
                continue;
            }
            seen.insert(next);
            if (next == to) {
                return true;
            }
            if (!board.passable(next)) {
                continue;
            }
            queue.push(next);
        }
    }
    return false;
}
