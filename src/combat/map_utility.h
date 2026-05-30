#ifndef MAP_UTILITY_H
#define MAP_UTILITY_H

#include "core/board.h"

#include <QVector>

QVector<Hex> line(const Board& board, const Hex& from, const Hex& to);
bool blockedBetween(const Board& board, const Hex& from, const Hex& to);
bool pathExists(const Board& board, const Hex& from, const Hex& to);

#endif // MAP_UTILITY_H
