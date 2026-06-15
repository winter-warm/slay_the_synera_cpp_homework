#include "obstacle.h"

Obstacle::Obstacle(bool blockAttack, int image)
    : Unit("Obstacle")
    , blockAttackValue(blockAttack)
    , imageValue(image) {}

void Obstacle::update(float, Board&) {}
