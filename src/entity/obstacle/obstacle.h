#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "entity/unit.h"

class Obstacle : public Unit {
public:
    explicit Obstacle(bool blockAttack = true, int image = 1);

    bool getBlockAttack() const { return blockAttackValue; }
    int getImage() const { return imageValue; }
    void setBlockAttack(bool blockAttack) { blockAttackValue = blockAttack; }
    void setImage(int image) { imageValue = image; }

    void update(float deltatime,const Board& board) override;

private:
    bool blockAttackValue;
    int imageValue;
};

#endif // OBSTACLE_H
