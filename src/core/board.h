#ifndef BOARD_H
#define BOARD_H

#include <QHash>
#include <QString>
#include <QVector>
#include <memory>
#include <vector>
#include "hex.h"

class QJsonArray;
class Obstacle;
class Unit;

class Board {
public:
    enum class Zone { None, Player, Enemy, Neutral, BlockedObstacle, OpenObstacle };

    Board();
    ~Board();

    bool load(const QString& path);

    QString id() const { return boardId; }
    QVector<Hex> cells() const;

    bool has(const Hex& h) const;
    bool empty(const Hex& h) const;
    bool passable(const Hex& h) const;
    Zone zone(const Hex& h) const;
    Unit* unitAt(const Hex& h) const;
    bool posOf(Unit* unit, Hex* out) const;

    bool add(Unit* unit, const Hex& h);
    bool move(Unit* unit, const Hex& h);
    void remove(Unit* unit);
    void clearUnits();
    QVector<Unit*> units() const;

    int dist(const Hex& a, const Hex& b) const;
    QVector<Hex> neighbors(const Hex& h) const;
    bool near(const Hex& a, const Hex& b) const;

private:
    struct Cell {
        Unit* unit = nullptr; bool pass = true; Zone zone = Zone::None;
    };

    bool build(const QJsonArray& rows, const QJsonArray& obstacleRows);
    void addObstacle(const Hex& h, bool blockAttack, int image);

    QString boardId;
    QHash<Hex, Cell> cellMap;
    QHash<Unit*, Hex> unitPos;
    std::vector<std::unique_ptr<Obstacle>> obstacles;
};

#endif // BOARD_H
