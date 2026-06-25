#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "entity/component/spritecomponent.h"
#include "entity/unit.h"

class Obstacle : public Unit {
public:
    explicit Obstacle(bool blockAttack = true, int image = 1, const std::string& imagePath = "");

    bool blocksAttack() const { return blockAttackValue; }
    int getImage() const { return imageValue; }
    void setBlockAttack(bool blockAttack) { blockAttackValue = blockAttack; }
    void setImage(int image) { imageValue = image; }
    void setImagePath(const std::string& imagePath) { spriteComponent.setImagePath(imagePath); }
    SpriteComponent& sprite() { return spriteComponent; }
    const SpriteComponent& sprite() const { return spriteComponent; }

    void update(float deltatime, Board& board) override;

private:
    bool blockAttackValue;
    int imageValue;
    SpriteComponent spriteComponent;
};

#endif // OBSTACLE_H
