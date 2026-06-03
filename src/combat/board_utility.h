#ifndef BOARD_UTILITY_H
#define BOARD_UTILITY_H

#include "core/board.h"

#include <vector>

std::vector<Hex> line(const Board& board, const Hex& from, const Hex& to);
bool blockedBetween(const Board& board, const Hex& from, const Hex& to);
bool pathExists(const Board& board, const Hex& from, const Hex& to);

#endif // BOARD_UTILITY_H
