#include "gamehud.h"
#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
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
    button->setAutoRaise(false);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(R"(
        QToolButton {
            background: rgba(244, 226, 183, 245);
            border: 1px solid rgba(92, 55, 18, 180);
            border-radius: 8px;
        }
        QToolButton:hover {
            background-color: rgba(255, 239, 196, 235);
        }
        QToolButton:pressed {
            background-color: rgba(199, 158, 91, 220);
        }
    )");
    return button;
}

static QString formatElapsed(int seconds) {
    seconds = qMax(0, seconds);
    const int hours = seconds / 3600;
    const int minutes = (seconds % 3600) / 60;
    const int secs = seconds % 60;
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}

GameHud::GameHud(QWidget* parent)
    : QWidget(parent)
    , hpIconLabel(makeIconLabel(this, "ui_hp.png", QSize(42, 42)))
    , hpLabel(new QLabel(this))
    , coinIconLabel(makeIconLabel(this, "ui_coin.png", QSize(42, 42)))
    , goldLabel(new QLabel(this))
    , timeLabel(new QLabel(this))
    , exitButton(makeHudButton(this, "bottom_exit.png", QSize(118, 58), QSize(112, 52)))
    , bagButton(makeHudButton(this, "bottom_bag.png", QSize(58, 58), QSize(48, 52)))
    , shopButton(makeHudButton(this, "bottom_shop.png", QSize(58, 58), QSize(48, 52))) {
    setFixedHeight(72);
    setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 6, 16, 6);
    layout->setSpacing(8);

    hpLabel->setMinimumWidth(82);
    goldLabel->setMinimumWidth(58);
    timeLabel->setMinimumWidth(96);
    hpLabel->setStyleSheet("font-size: 18px; font-weight: 900; color: #1e1106; background: transparent;");
    goldLabel->setStyleSheet("font-size: 18px; font-weight: 900; color: #1e1106; background: transparent;");
    timeLabel->setStyleSheet("font-size: 18px; font-weight: 900; color: #1e1106; background: transparent;");

    layout->addWidget(hpIconLabel);
    layout->addWidget(hpLabel);
    layout->addSpacing(20);
    layout->addWidget(coinIconLabel);
    layout->addWidget(goldLabel);
    layout->addSpacing(20);
    layout->addWidget(timeLabel);
    layout->addStretch();
    layout->addWidget(exitButton);
    layout->addWidget(bagButton);
    layout->addWidget(shopButton);

    connect(bagButton, &QToolButton::clicked, this, [this]() {
        emit bagRequested();
    });
    connect(shopButton, &QToolButton::clicked, this, [this]() {
        emit shopRequested();
    });
    connect(exitButton, &QToolButton::clicked, this, [this]() {
        emit returnToStartRequested();
    });

    setStyleSheet(R"(
        GameHud {
            background-color: #f7ead0;
            border-bottom: 2px solid #9d7134;
            color: #1e1106;
        }
    )");
}

void GameHud::setState(const GameState& state) {
    const bool goldIncreased = lastGold >= 0 && state.gold > lastGold;
    hpLabel->setText(QString("%1/%2").arg(state.playerHp).arg(state.maxPlayerHp));
    goldLabel->setText(QString::number(state.gold));
    setElapsedSeconds(state.elapsedSeconds);
    lastGold = state.gold;
    if (goldIncreased) {
        animateGoldGain();
    }
}

void GameHud::setElapsedSeconds(int seconds) {
    timeLabel->setText(formatElapsed(seconds));
}

void GameHud::animateGoldGain() {
    auto bounceWidget = [](QWidget* widget, int lift) {
        if (!widget) {
            return;
        }
        auto* group = new QSequentialAnimationGroup(widget);
        const QPoint start = widget->pos();
        auto* up = new QPropertyAnimation(widget, "pos", group);
        up->setDuration(95);
        up->setStartValue(start);
        up->setEndValue(start + QPoint(0, -lift));
        up->setEasingCurve(QEasingCurve::OutCubic);
        auto* down = new QPropertyAnimation(widget, "pos", group);
        down->setDuration(135);
        down->setStartValue(start + QPoint(0, -lift));
        down->setEndValue(start);
        down->setEasingCurve(QEasingCurve::OutBounce);
        group->addAnimation(up);
        group->addAnimation(down);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    };

    bounceWidget(coinIconLabel, 9);
    bounceWidget(goldLabel, 7);
}
