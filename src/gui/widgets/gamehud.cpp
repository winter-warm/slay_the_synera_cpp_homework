#include "gamehud.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QToolButton>

static QString uiAsset(const QString& fileName) {
    return QDir(QCoreApplication::applicationDirPath()).filePath("assets/ui/" + fileName);
}

static QLabel* makeIconLabel(QWidget* parent, const QString& fileName, const QSize& size) {
    auto* label = new QLabel(parent);
    label->setFixedSize(size);
    label->setScaledContents(true);
    label->setPixmap(QPixmap(uiAsset(fileName)));
    return label;
}

static QToolButton* makeHudButton(QWidget* parent, const QString& fileName, const QSize& size, const QSize& iconSize) {
    auto* button = new QToolButton(parent);
    button->setIcon(QIcon(uiAsset(fileName)));
    button->setIconSize(iconSize);
    button->setFixedSize(size);
    button->setAutoRaise(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(R"(
        QToolButton {
            background: transparent;
            border: none;
        }
        QToolButton:hover {
            background-color: rgba(255,255,255,28);
            border-radius: 8px;
        }
        QToolButton:pressed {
            background-color: rgba(0,0,0,35);
            border-radius: 8px;
        }
    )");
    return button;
}

GameHud::GameHud(QWidget* parent)
    : QWidget(parent)
    , hpIconLabel(makeIconLabel(this, "ui_hp.png", QSize(42, 42)))
    , hpLabel(new QLabel(this))
    , coinIconLabel(makeIconLabel(this, "ui_coin.png", QSize(42, 42)))
    , goldLabel(new QLabel(this))
    , exitButton(makeHudButton(this, "bottom_exit.png", QSize(118, 58), QSize(112, 52)))
    , bagButton(makeHudButton(this, "bottom_bag.png", QSize(58, 58), QSize(48, 52)))
    , shopButton(makeHudButton(this, "bottom_shop.png", QSize(58, 58), QSize(48, 52))) {
    setFixedHeight(72);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 6, 16, 6);
    layout->setSpacing(8);

    hpLabel->setMinimumWidth(82);
    goldLabel->setMinimumWidth(58);
    hpLabel->setStyleSheet("font-size: 18px; font-weight: 700; color: #f7efe1;");
    goldLabel->setStyleSheet("font-size: 18px; font-weight: 700; color: #f7efe1;");

    layout->addWidget(hpIconLabel);
    layout->addWidget(hpLabel);
    layout->addSpacing(20);
    layout->addWidget(coinIconLabel);
    layout->addWidget(goldLabel);
    layout->addStretch();
    layout->addWidget(exitButton);
    layout->addWidget(bagButton);
    layout->addWidget(shopButton);

    connect(bagButton, &QToolButton::clicked, this, [this]() {
        // TODO(hextech-bag): display selected HexTechCardRecord entries here; remove this comment after bag UI is implemented.
        QMessageBox::information(this, "Bag", "Bag panel placeholder.");
    });
    connect(shopButton, &QToolButton::clicked, this, [this]() {
        QMessageBox::information(this, "Shop", "Shop panel placeholder.");
    });
    connect(exitButton, &QToolButton::clicked, qApp, &QApplication::quit);

    setStyleSheet(R"(
        GameHud {
            background-color: #242424;
            color: #f4f4f4;
        }
    )");
}

void GameHud::setState(const GameState& state) {
    hpLabel->setText(QString("%1/%2").arg(state.playerHp).arg(state.maxPlayerHp));
    goldLabel->setText(QString::number(state.gold));
}
