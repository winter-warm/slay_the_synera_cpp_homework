#ifndef UNIT_H
#define UNIT_H

#include "core/hex.h"
#include <string>

class Board;
class Unit{
private:
    static int nextId;

    int unitId;
    std::string unitName;
    Hex unitPos;
    bool hasPos;

public:
    explicit Unit(const std::string& name = "Unit");
    virtual ~Unit() = default;

    int id() const { return unitId; }
    std::string name() const { return unitName; }
    bool hasPosition() const { return hasPos; }
    Hex position() const { return unitPos; }

    void setName(const std::string& name) { unitName = name; }
    void setPosition(const Hex& h) { unitPos = h; hasPos = true; }
    void clearPosition() { hasPos = false; }

    virtual void update(float deltatime, Board& board) = 0;
};

#endif // UNIT_H
