#include "obstacle.h"

Obstacle::Obstacle(bool blockAttack, int image, const std::string& imagePath)
    : Unit("Obstacle")
    , blockAttackValue(blockAttack)
    , imageValue(image)
    , spriteComponent(imagePath, {}, 62, 62, -31, -50) {}

void Obstacle::update(float, Board&) {}
