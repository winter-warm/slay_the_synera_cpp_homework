#ifndef OBJECT_H
#define OBJECT_H

#include "entity/component/interactcomponent.h"
#include "entity/component/spritecomponent.h"
#include "entity/unit.h"
#include <string>

class Object : public Unit
{
public:
    Object(std::string objectType = "object",
           std::string closedImagePath = {},
           std::string openImagePath = {});

    const std::string& objectType() const { return objectTypeValue; }
    bool opened() const { return openedValue; }
    void open();
    SpriteComponent& sprite() { return spriteComponent; }
    const SpriteComponent& sprite() const { return spriteComponent; }
    InteractComponent& interact() { return interactComponent; }
    const InteractComponent& interact() const { return interactComponent; }
    void update(float deltatime, Board& board) override;

private:
    std::string objectTypeValue;
    bool openedValue = false;
    SpriteComponent spriteComponent;
    InteractComponent interactComponent;
};

#endif // OBJECT_H
