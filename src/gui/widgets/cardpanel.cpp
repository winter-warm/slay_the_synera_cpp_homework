#include "cardpanel.h"
#include "entity/character/characterfactory.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cmath>

static QString uiAssetPath(const QString& fileName) {
    return QDir(QCoreApplication::applicationDirPath()).filePath("assets/ui/" + fileName);
}

static QString characterCardPath(const std::string& name) {
    const QString relative = QDir("assets/characters").filePath(QString::fromStdString(name) + "/card.png");
    const QString runtimePath = QDir(QCoreApplication::applicationDirPath()).filePath(relative);
    if (QFile::exists(runtimePath)) {
        return runtimePath;
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(relative);
}

static int costForRarity(int rarity) {
    if (rarity == 2) {
        return 3;
    }
    if (rarity == 3) {
        return 5;
    }
    if (rarity == 4) {
        return 10;
    }
    return 1;
}

static QString starsForLevel(int starLevel) {
    QString text;
    for (int i = 0; i < std::max(1, std::min(3, starLevel)); ++i) {
        text += QString::fromUtf8("★");
    }
    return text;
}

CardPanel::CardPanel(QWidget* parent)
    : QFrame(parent)
    , titleLabel(new QLabel(this))
    , expLabel(new QLabel(this))
    , expFill(new QFrame(this))
    , cardsLayout(new QGridLayout())
    , closeButton(new QPushButton("X", this)) {
    setObjectName("cardPanel");
    setAttribute(Qt::WA_StyledBackground, true);
    setVisible(false);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 18, 28, 24);
    root->setSpacing(12);

    auto* top = new QHBoxLayout();
    titleLabel->setStyleSheet("font-size: 24px; font-weight: 800; color: #332411; background: transparent;");
    expLabel->setStyleSheet("font-size: 15px; font-weight: 700; color: #4a3215; background: transparent;");
    closeButton->setFixedSize(34, 30);
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setStyleSheet("QPushButton { background: rgba(64,35,12,140); color: #f8ebd0; border: 1px solid #d1a45c; border-radius: 4px; font-weight: 800; }");
    top->addWidget(titleLabel);
    top->addStretch();
    top->addWidget(expLabel);
    top->addWidget(closeButton);
    root->addLayout(top);

    auto* expTrack = new QFrame(this);
    expTrack->setFixedHeight(12);
    expTrack->setObjectName("expTrack");
    expFill->setParent(expTrack);
    expFill->setObjectName("expFill");
    expTrack->setStyleSheet("#expTrack { background: rgba(80, 45, 16, 95); border: 1px solid rgba(80,45,16,150); border-radius: 5px; } #expFill { background: #e7b94d; border-radius: 4px; }");
    root->addWidget(expTrack);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    auto* cardsHost = new QWidget(scroll);
    cardsHost->setAttribute(Qt::WA_TranslucentBackground);
    cardsLayout->setContentsMargins(0, 4, 0, 0);
    cardsLayout->setSpacing(14);
    cardsHost->setLayout(cardsLayout);
    scroll->setWidget(cardsHost);
    root->addWidget(scroll, 1);

    connect(closeButton, &QPushButton::clicked, this, &CardPanel::closePanel);

    setStyleSheet(QString(R"(
        #cardPanel {
            border-image: url("%1") 36 36 36 36 stretch stretch;
            background-color: rgba(222, 186, 122, 230);
            border-radius: 8px;
        }
    )").arg(uiAssetPath("shop_panel_parchment.png")));
}

void CardPanel::setGameState(const GameState& state) {
    currentState = state;
    if (currentMode != Mode::None) {
        rebuild();
    }
}

void CardPanel::showShop() {
    currentMode = Mode::Shop;
    selectedBagIndexes.clear();
    rebuild();
    relayoutAnimated(true);
}

void CardPanel::showBag() {
    currentMode = Mode::Bag;
    selectedBagIndexes.clear();
    rebuild();
    relayoutAnimated(true);
}

void CardPanel::closePanel() {
    if (currentMode == Mode::None && !isVisible()) {
        return;
    }
    currentMode = Mode::None;
    selectedBagIndexes.clear();
    const QRect end = hiddenGeometry();
    auto* animation = new QPropertyAnimation(this, "geometry", this);
    animation->setDuration(180);
    animation->setStartValue(geometry());
    animation->setEndValue(end);
    animation->setEasingCurve(QEasingCurve::InCubic);
    connect(animation, &QPropertyAnimation::finished, this, [this, animation]() {
        hide();
        animation->deleteLater();
    });
    animation->start();
}

void CardPanel::toggleShop() {
    if (currentMode == Mode::Shop && isVisible()) {
        closePanel();
        return;
    }
    showShop();
}

void CardPanel::toggleBag() {
    if (currentMode == Mode::Bag && isVisible()) {
        closePanel();
        return;
    }
    showBag();
}

void CardPanel::relayoutAnimated(bool animated) {
    const QRect end = shownGeometry();
    const QRect start = hiddenGeometry();
    if (!animated) {
        setGeometry(isVisible() ? end : start);
        return;
    }
    setGeometry(start);
    show();
    raise();
    auto* animation = new QPropertyAnimation(this, "geometry", this);
    animation->setDuration(220);
    animation->setStartValue(start);
    animation->setEndValue(end);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(animation, &QPropertyAnimation::finished, animation, &QPropertyAnimation::deleteLater);
    animation->start();
}

void CardPanel::rebuild() {
    while (QLayoutItem* item = cardsLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            delete widget;
        }
        delete item;
    }
    if (currentMode == Mode::Shop) {
        rebuildShop();
    } else if (currentMode == Mode::Bag) {
        rebuildBag();
    }
}

void CardPanel::rebuildShop() {
    titleLabel->setText(QString::fromUtf8("商店"));
    const int threshold = shopExpThreshold();
    expLabel->setText(threshold > 0
                          ? QString::fromUtf8("等级 %1  %2/%3").arg(currentState.shop.level).arg(currentState.shop.exp).arg(threshold)
                          : QString::fromUtf8("等级 %1  MAX").arg(currentState.shop.level));
    const int trackWidth = std::max(1, width() - 56);
    const double ratio = threshold > 0 ? std::min(1.0, currentState.shop.exp / static_cast<double>(threshold)) : 1.0;
    expFill->setGeometry(1, 1, static_cast<int>((trackWidth - 2) * ratio), 10);

    for (int i = 0; i < static_cast<int>(currentState.shop.offers.size()); ++i) {
        const ShopOffer& offer = currentState.shop.offers[static_cast<size_t>(i)];
        const auto info = characterfactory::infoFor(offer.templateId);
        const int cost = info ? costForRarity(info->rarity) : 0;
        cardsLayout->addWidget(createCharacterCard(offer.templateId, 1, cost, offer.sold, i, false), 0, i);
    }
}

void CardPanel::rebuildBag() {
    titleLabel->setText(QString::fromUtf8("背包"));
    expLabel->setText(QString::fromUtf8("持有 %1 张").arg(currentState.ownedCharacterCards.size()));
    expFill->setGeometry(1, 1, width() - 58, 10);

    for (int i = 0; i < static_cast<int>(currentState.ownedCharacterCards.size()); ++i) {
        const OwnedCharacterCard& card = currentState.ownedCharacterCards[static_cast<size_t>(i)];
        cardsLayout->addWidget(createCharacterCard(card.templateId, card.starLevel, 0, false, i, true), i / 4, i % 4);
    }
}

QWidget* CardPanel::createCharacterCard(int templateId, int starLevel, int cost, bool sold, int index, bool bagCard) {
    auto* frame = new QFrame(this);
    frame->setFixedSize(196, 314);
    frame->setObjectName("characterCard");
    frame->setProperty("selected", selectedBagIndexes.contains(index));
    frame->setCursor(bagCard ? Qt::PointingHandCursor : Qt::ArrowCursor);
    frame->setStyleSheet(QString(R"(
        #characterCard {
            border-image: url("%1") 28 28 28 28 stretch stretch;
            background-color: rgba(235, 206, 151, 235);
            border-radius: 6px;
        }
        #characterCard[selected="true"] {
            background-color: rgba(246, 213, 126, 250);
            border: 2px solid #d5962e;
        }
        QLabel { background: transparent; color: #2d2112; }
    )").arg(uiAssetPath("card_parchment.png")));

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(12, 10, 12, 12);
    layout->setSpacing(5);

    const auto info = characterfactory::infoFor(templateId);
    const QString displayName = info ? QString::fromStdString(info->displayName) : QString::fromUtf8("未知角色");
    const QString magicName = info ? QString::fromStdString(info->magicName) : QString();
    const QString skillDescription = info ? QString::fromStdString(info->skillDescription) : QString();
    const int maxHp = info ? info->maxHp : 0;
    const int attack = info ? info->attack : 0;
    const int defense = info ? info->defense : 0;

    auto* stars = new QLabel(starsForLevel(starLevel), frame);
    stars->setAlignment(Qt::AlignCenter);
    stars->setToolTip(QString::fromUtf8("每次升星使最大生命、攻击、防御 +20%，最高三星"));
    stars->setStyleSheet("font-size: 17px; font-weight: 800; color: #9b5d16; background: transparent;");
    layout->addWidget(stars);

    auto* name = new QLabel(displayName, frame);
    name->setAlignment(Qt::AlignCenter);
    name->setWordWrap(true);
    name->setStyleSheet("font-size: 17px; font-weight: 900;");
    layout->addWidget(name);

    auto* image = new QLabel(frame);
    image->setFixedSize(132, 108);
    image->setAlignment(Qt::AlignCenter);
    if (info) {
        QPixmap pixmap(characterCardPath(info->name));
        if (!pixmap.isNull()) {
            image->setPixmap(pixmap.scaled(image->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
    }
    layout->addWidget(image, 0, Qt::AlignCenter);

    const double multiplier = 1.0 + 0.2 * (std::max(1, std::min(3, starLevel)) - 1);
    auto* stats = new QLabel(QString::fromUtf8("生命 %1  攻击 %2  防御 %3")
                                 .arg(std::lround(maxHp * multiplier))
                                 .arg(std::lround(attack * multiplier))
                                 .arg(std::lround(defense * multiplier)),
                             frame);
    stats->setAlignment(Qt::AlignCenter);
    stats->setWordWrap(true);
    stats->setStyleSheet("font-size: 12px; font-weight: 800;");
    layout->addWidget(stats);

    auto* skill = new QLabel(magicName.isEmpty() ? skillDescription : QString::fromUtf8("%1：%2").arg(magicName, skillDescription), frame);
    skill->setWordWrap(true);
    skill->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    skill->setMinimumHeight(54);
    skill->setStyleSheet("font-size: 12px;");
    layout->addWidget(skill, 1);

    if (bagCard) {
        auto* button = new QPushButton(selectedBagIndexes.contains(index) ? QString::fromUtf8("已选择") : QString::fromUtf8("选择合成"), frame);
        button->setEnabled(starLevel < 3);
        button->setStyleSheet("QPushButton { padding: 5px; background: rgba(91,55,22,160); color: #fff0d1; border-radius: 4px; } QPushButton:disabled { color: #7c6b55; background: rgba(91,55,22,60); }");
        layout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, index]() {
            if (selectedBagIndexes.contains(index)) {
                selectedBagIndexes.remove(index);
            } else {
                selectedBagIndexes.insert(index);
            }
            if (selectedBagIndexes.size() >= 2) {
                const QList<int> values = selectedBagIndexes.values();
                emit mergeRequested(values[0], values[1]);
                selectedBagIndexes.clear();
            }
            rebuild();
        });
    } else {
        auto* button = new QPushButton(sold ? QString::fromUtf8("已售出") : QString::fromUtf8("花费 %1").arg(cost), frame);
        button->setEnabled(!sold && templateId > 0);
        button->setStyleSheet("QPushButton { padding: 6px; background: rgba(91,55,22,180); color: #fff0d1; border-radius: 4px; font-weight: 800; } QPushButton:disabled { color: #7c6b55; background: rgba(91,55,22,70); }");
        layout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, index]() {
            emit buyRequested(index);
        });
    }

    return frame;
}

QRect CardPanel::shownGeometry() const {
    QWidget* host = parentWidget();
    if (!host) {
        return QRect();
    }
    const int top = 72;
    const int panelWidth = static_cast<int>(host->width() * 0.70);
    const int panelHeight = static_cast<int>((host->height() - top) * 0.70);
    const int x = (host->width() - panelWidth) / 2;
    const int y = top + (host->height() - top - panelHeight) / 2;
    return QRect(x, y, panelWidth, panelHeight);
}

QRect CardPanel::hiddenGeometry() const {
    QRect shown = shownGeometry();
    shown.moveTop(-shown.height() - 12);
    return shown;
}

int CardPanel::shopExpThreshold() const {
    if (currentState.shop.level == 1) {
        return 10;
    }
    if (currentState.shop.level == 2) {
        return 15;
    }
    if (currentState.shop.level == 3) {
        return 20;
    }
    return 0;
}
