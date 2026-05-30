#include "characterhud.h"
#include <QFont>
#include <QPainter>
#include <QPolygonF>
#include <QString>

CharacterHUD::CharacterHUD(const std::string& displayName)
    : displayNameValue(displayName)
    , fillColor(90, 145, 205)
    , visible(true) {}

void CharacterHUD::setDisplayName(const std::string& displayName) {
    displayNameValue = displayName;
}

std::string CharacterHUD::displayName() const {
    return displayNameValue;
}

void CharacterHUD::setVisible(bool shown) {
    visible = shown;
}

bool CharacterHUD::isVisible() const {
    return visible;
}

void CharacterHUD::paint(QPainter* painter, const QRectF& bounds) const {
    if (!visible || !painter) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(20, 20, 20, 120));
    painter->drawEllipse(QRectF(bounds.center().x() - 18, bounds.bottom() - 22, 36, 12));

    const QPointF center = bounds.center();
    QPolygonF badge;
    badge << QPointF(center.x(), center.y() - 27)
          << QPointF(center.x() + 24, center.y() - 10)
          << QPointF(center.x() + 20, center.y() + 20)
          << QPointF(center.x(), center.y() + 31)
          << QPointF(center.x() - 20, center.y() + 20)
          << QPointF(center.x() - 24, center.y() - 10);

    painter->setPen(QPen(QColor(18, 18, 18), 2));
    painter->setBrush(fillColor);
    painter->drawPolygon(badge);

    painter->setPen(Qt::white);
    QFont font = painter->font();
    font.setPointSize(12);
    font.setBold(true);
    painter->setFont(font);
    const QString displayText = QString::fromStdString(displayNameValue);
    painter->drawText(QRectF(center.x() - 18, center.y() - 18, 36, 36),
                      Qt::AlignCenter,
                      displayText.left(1));
}
