#ifndef OBJECT_H
#define OBJECT_H

#include "entity/unit.h"
#include <string>

class ObjectRenderComponent {
public:
    ObjectRenderComponent(std::string closedImagePath = {}, std::string openImagePath = {});

    const std::string& imagePath(bool opened) const;
    void setImagePaths(std::string closedImagePath, std::string openImagePath);
    bool isVisible() const { return visible; }
    void setVisible(bool value) { visible = value; }

private:
    std::string closedPath;
    std::string openPath;
    bool visible = true;
};

class ObjectInteractComponent {
public:
    bool canInteract() const { return enabled && !used; }
    bool tryInteract();
    void setEnabled(bool value) { enabled = value; }
    void reset();

private:
    bool enabled = true;
    bool used = false;
};

class Object : public Unit
{
public:
    Object(std::string objectType = "object",
           std::string closedImagePath = {},
           std::string openImagePath = {});

    const std::string& objectType() const { return objectTypeValue; }
    bool opened() const { return openedValue; }
    void open();
    ObjectRenderComponent& render() { return renderComponent; }
    const ObjectRenderComponent& render() const { return renderComponent; }
    ObjectInteractComponent& interact() { return interactComponent; }
    const ObjectInteractComponent& interact() const { return interactComponent; }
    void update(float deltatime, Board& board) override;

private:
    std::string objectTypeValue;
    bool openedValue = false;
    ObjectRenderComponent renderComponent;
    ObjectInteractComponent interactComponent;
};

#endif // OBJECT_H
