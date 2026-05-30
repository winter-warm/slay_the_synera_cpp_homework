#include "unit.h"

int Unit::nextId = 0;

Unit::Unit(const std::string& name)
    : unitId(nextId++)
    , unitName(name)
    , unitPos()
    , hasPos(false) {}
