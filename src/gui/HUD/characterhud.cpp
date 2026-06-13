#include "characterhud.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QPainter>
#include <QPolygonF>
#include <QString>

CharacterHUD::CharacterHUD(const std::string& displayName)
    : displayNameValue(displayName)
    , fillColor(90, 145, 205)
    , visible(true) {
    loadPawnImage();
}

void CharacterHUD::setDisplayName(const std::string& displayName) {
    displayNameValue = displayName;
    loadPawnImage();
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

void CharacterHUD::loadPawnImage() {
    pawnImage = QPixmap();

    const QString characterName = QString::fromStdString(displayNameValue);
    if (characterName.isEmpty()) {
        return;
    }

    const QString relativePath = QDir("assets/characters").filePath(characterName + "/pawn.png");
    const QString runtimePath = QDir(QCoreApplication::applicationDirPath()).filePath(relativePath);
    const QString sourcePath = QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath("../../../" + relativePath);

    for (const QString& path : {runtimePath, sourcePath}) {
        if (QFile::exists(path) && pawnImage.load(path)) {
            return;
        }
    }
}

void CharacterHUD::paint(QPainter* painter, const QRectF& bounds) const {
    if (!visible || !painter) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(20, 20, 20, 120));
    painter->drawEllipse(QRectF(bounds.center().x() - 18, bounds.bottom() - 22, 36, 12));

    if (!pawnImage.isNull()) {
        const QSizeF targetSize(72.0, 72.0);
        QPixmap scaled = pawnImage.scaled(targetSize.toSize(),
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
        const QPointF topLeft(bounds.center().x() - scaled.width() * 0.5,
                              bounds.center().y() - scaled.height() * 0.5);
        painter->drawPixmap(topLeft, scaled);
        return;
    }

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
