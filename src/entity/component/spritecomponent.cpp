#include "spritecomponent.h"

#include <utility>

SpriteComponent::SpriteComponent(std::string imagePath,
                                 std::string alternateImagePath,
                                 int width,
                                 int height,
                                 int offsetX,
                                 int offsetY)
    : primaryPath(std::move(imagePath))
    , alternatePath(std::move(alternateImagePath))
    , widthValue(width)
    , heightValue(height)
    , offsetXValue(offsetX)
    , offsetYValue(offsetY)
{}

const std::string& SpriteComponent::imagePath(bool alternate) const
{
    return alternate ? alternatePath : primaryPath;
}

void SpriteComponent::setImagePath(std::string imagePath)
{
    primaryPath = std::move(imagePath);
}

void SpriteComponent::setImagePaths(std::string imagePath, std::string alternateImagePath)
{
    primaryPath = std::move(imagePath);
    alternatePath = std::move(alternateImagePath);
}

void SpriteComponent::setTextureGeometry(int width, int height, int offsetX, int offsetY)
{
    widthValue = width;
    heightValue = height;
    offsetXValue = offsetX;
    offsetYValue = offsetY;
}
