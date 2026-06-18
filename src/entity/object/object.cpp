#include "object.h"

#include <utility>

ObjectRenderComponent::ObjectRenderComponent(std::string closedImagePath, std::string openImagePath)
    : closedPath(std::move(closedImagePath))
    , openPath(std::move(openImagePath))
{}

const std::string& ObjectRenderComponent::imagePath(bool opened) const
{
    return opened ? openPath : closedPath;
}

void ObjectRenderComponent::setImagePaths(std::string closedImagePath, std::string openImagePath)
{
    closedPath = std::move(closedImagePath);
    openPath = std::move(openImagePath);
}

bool ObjectInteractComponent::tryInteract()
{
    if (!canInteract()) {
        return false;
    }
    used = true;
    return true;
}

void ObjectInteractComponent::reset()
{
    used = false;
}

Object::Object(std::string objectType, std::string closedImagePath, std::string openImagePath)
    : Unit(objectType)
    , objectTypeValue(std::move(objectType))
    , renderComponent(std::move(closedImagePath), std::move(openImagePath))
{}

void Object::open()
{
    openedValue = true;
    interactComponent.setEnabled(false);
}

void Object::update(float, Board&)
{}
