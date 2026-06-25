#ifndef SPRITECOMPONENT_H
#define SPRITECOMPONENT_H

#include <string>

class SpriteComponent {
public:
    SpriteComponent(std::string imagePath = {},
                    std::string alternateImagePath = {},
                    int width = 62,
                    int height = 62,
                    int offsetX = -31,
                    int offsetY = -50);

    const std::string& imagePath(bool alternate = false) const;
    void setImagePath(std::string imagePath);
    void setImagePaths(std::string imagePath, std::string alternateImagePath);

    bool isVisible() const { return visible; }
    void setVisible(bool value) { visible = value; }

    int textureWidth() const { return widthValue; }
    int textureHeight() const { return heightValue; }
    int textureOffsetX() const { return offsetXValue; }
    int textureOffsetY() const { return offsetYValue; }
    void setTextureGeometry(int width, int height, int offsetX, int offsetY);

private:
    std::string primaryPath;
    std::string alternatePath;
    bool visible = true;
    int widthValue;
    int heightValue;
    int offsetXValue;
    int offsetYValue;
};

#endif // SPRITECOMPONENT_H
