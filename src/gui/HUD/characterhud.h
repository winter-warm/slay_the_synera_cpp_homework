#ifndef CHARACTERHUD_H
#define CHARACTERHUD_H

#include <QColor>
#include <QPixmap>
#include <QRectF>
#include <string>

class QPainter;

class CharacterHUD {
public:
    explicit CharacterHUD(const std::string& displayName = "Unit");

    void setDisplayName(const std::string& displayName);
    std::string displayName() const;

    void setVisible(bool shown);
    bool isVisible() const;

    void paint(QPainter* painter, const QRectF& bounds) const;

private:
    void loadPawnImage();

    std::string displayNameValue;
    QColor fillColor;
    QPixmap pawnImage;
    bool visible;
};

#endif // CHARACTERHUD_H
