#include "object.h"

#include <utility>

Object::Object(std::string objectType, std::string closedImagePath, std::string openImagePath)
    : Unit(objectType)
    , objectTypeValue(std::move(objectType))
    , spriteComponent(std::move(closedImagePath), std::move(openImagePath), 76, 76, -38, -42)
{}

void Object::open()
{
    openedValue = true;
    interactComponent.setEnabled(false);
}

void Object::update(float, Board&)
{}
