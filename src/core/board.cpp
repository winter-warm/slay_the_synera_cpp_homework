#include "board.h"
#include "entity/obstacle/obstacle.h"
#include "entity/unit.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <cmath>
#include <utility>

static Board::Zone zoneFromChar(QChar ch) {
    switch (ch.toLatin1()) {
    case '1':
        return Board::Zone::Player;
    case '2':
        return Board::Zone::Enemy;
    case '3':
        return Board::Zone::Neutral;
    case '4':
        return Board::Zone::BlockedObstacle;
    case '5':
        return Board::Zone::OpenObstacle;
    default:
        return Board::Zone::None;
    }
}

Board::Board() = default;

Board::~Board() = default;

bool Board::load(const std::string& path) {
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open board map:" << file.errorString();
        return false;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse board map:" << error.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qDebug() << "board map root must be an object";
        return false;
    }

    const QJsonObject root = doc.object();
    boardId = root.value("id").toString().toStdString();
    return build(root.value("map").toArray(), root.value("obstacles").toArray());
}

bool Board::build(const QJsonArray& rows, const QJsonArray& obstacleRows) {
    std::unordered_map<Hex, Cell> next;
    for (int row = 0; row < rows.size(); ++row) {
        const QString line = rows.at(row).toString();
        for (int col = 0; col < line.size(); ++col) {
            const QChar ch = line.at(col);
            if (ch == '0') {
                continue;
            }

            const Hex h{col, -col - row, row};
            if (!isValidHex(h) || next.find(h) != next.end()) {
                qDebug() << "invalid or duplicate board cell";
                return false;
            }

            Cell cell; cell.pass = true; cell.zone = zoneFromChar(ch);
            next.emplace(h, cell);
        }
    }

    for (auto& entry : cellMap) {
        entry.second.unit = nullptr;
    }
    for (auto& entry : unitPos) {
        if (entry.first) {
            entry.first->clearPosition();
        }
    }
    unitPos.clear();
    obstacles.clear();
    cellMap = std::move(next);

    for (const Hex& h : cells()) {
        const Zone z = zone(h);
        if (z == Zone::BlockedObstacle) {
            addObstacle(h, true, 1);
        } else if (z == Zone::OpenObstacle) {
            addObstacle(h, false, 1);
        }
    }

    for (const QJsonValue& value : obstacleRows) {
        const QJsonObject obj = value.toObject();
        const int x = obj.value("x").toInt();
        const int z = obj.value("z").toInt();
        const Hex h{x, -x - z, z};
        if (!has(h)) {
            continue;
        }

        const bool blockAttack = obj.value("blockAttack").toBool(zone(h) != Zone::OpenObstacle);
        const int image = obj.value("image").toInt(1);
        Obstacle* existing = dynamic_cast<Obstacle*>(unitAt(h));
        if (existing) {
            existing->setBlockAttack(blockAttack);
            existing->setImage(image);
        } else {
            addObstacle(h, blockAttack, image);
        }
    }

    return true;
}

void Board::addObstacle(const Hex& h, bool blockAttack, int image) {
    std::unique_ptr<Obstacle> obstacle = std::make_unique<Obstacle>(blockAttack, image);
    Obstacle* ptr = obstacle.get();
    if (add(ptr, h)) {
        obstacles.push_back(std::move(obstacle));
    }
}

std::vector<Hex> Board::cells() const {
    std::vector<Hex> out;
    out.reserve(cellMap.size());
    for (const auto& entry : cellMap) {
        out.push_back(entry.first);
    }
    return out;
}

bool Board::has(const Hex& h) const {
    return cellMap.find(h) != cellMap.end();
}

bool Board::empty(const Hex& h) const {
    auto it = cellMap.find(h);
    return it != cellMap.end() && it->second.unit == nullptr;
}

bool Board::passable(const Hex& h) const {
    auto it = cellMap.find(h);
    return it != cellMap.end() && it->second.pass && it->second.unit == nullptr;
}

Board::Zone Board::zone(const Hex& h) const {
    auto it = cellMap.find(h);
    return it == cellMap.end() ? Zone::None : it->second.zone;
}

Unit* Board::unitAt(const Hex& h) const {
    auto it = cellMap.find(h);
    return it == cellMap.end() ? nullptr : it->second.unit;
}

bool Board::posOf(Unit* unit, Hex* out) const {
    auto it = unitPos.find(unit);
    if (it == unitPos.end() || !out) {
        return false;
    }
    *out = it->second;
    return true;
}

bool Board::add(Unit* unit, const Hex& h) {
    auto it = cellMap.find(h);
    if (!unit || it == cellMap.end() || !it->second.pass || it->second.unit || unitPos.find(unit) != unitPos.end()) {
        return false;
    }

    it->second.unit = unit;
    unitPos.emplace(unit, h);
    unit->setPosition(h);
    return true;
}

bool Board::move(Unit* unit, const Hex& h) {
    auto from = unitPos.find(unit); auto to = cellMap.find(h);
    if (!unit || from == unitPos.end() || to == cellMap.end() || !to->second.pass) {
        return false;
    }
    if (to->second.unit && to->second.unit != unit) {
        return false;
    }
    if (from->second == h) {
        return true;
    }

    cellMap[from->second].unit = nullptr;
    to->second.unit = unit;
    from->second = h;
    unit->setPosition(h);
    return true;
}

void Board::remove(Unit* unit) {
    auto it = unitPos.find(unit);
    if (!unit || it == unitPos.end()) {
        return;
    }

    auto cell = cellMap.find(it->second);
    if (cell != cellMap.end() && cell->second.unit == unit) {
        cell->second.unit = nullptr;
    }
    unitPos.erase(it);
    unit->clearPosition();
}

void Board::clearUnits() {
    std::vector<Unit*> list;
    for (const auto& entry : unitPos) {
        if (!dynamic_cast<Obstacle*>(entry.first)) {
            list.push_back(entry.first);
        }
    }
    for (Unit* unit : list) {
        remove(unit);
    }
}

std::vector<Unit*> Board::units() const {
    std::vector<Unit*> out;
    out.reserve(unitPos.size());
    for (const auto& entry : unitPos) {
        out.push_back(entry.first);
    }
    return out;
}

int Board::dist(const Hex& a, const Hex& b) const {
    return (std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z)) / 2;
}

std::vector<Hex> Board::neighbors(const Hex& h) const {
    static const Hex dirs[] = {
        {1, -1, 0},
        {1, 0, -1},
        {0, 1, -1},
        {-1, 1, 0},
        {-1, 0, 1},
        {0, -1, 1}
    };

    std::vector<Hex> out;
    out.reserve(6);
    for (const Hex& d : dirs) {
        const Hex n{h.x + d.x, h.y + d.y, h.z + d.z};
        if (has(n)) {
            out.push_back(n);
        }
    }
    return out;
}

bool Board::near(const Hex& a, const Hex& b) const {
    return dist(a, b) == 1;
}
