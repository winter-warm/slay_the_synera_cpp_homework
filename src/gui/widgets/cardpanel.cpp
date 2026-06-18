#include "cardpanel.h"

#include "entity/character/characterfactory.h"

#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QParallelAnimationGroup>
#include <QPalette>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStringList>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>

static QString uiAssetPath(const QString& fileName) {
    return QDir(QCoreApplication::applicationDirPath()).filePath("assets/ui/" + fileName);
}

static QString characterCardPath(const std::string& name) {
    const QString relative = QDir("assets/characters").filePath(QString::fromStdString(name) + "/card.png");
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

static QString bondDisplayName(const std::string& bond) {
    if (bond == "witch") {
        return QString::fromUtf8("魔女");
    }
    if (bond == "soldier") {
        return QString::fromUtf8("战士");
    }
    if (bond == "mage") {
        return QString::fromUtf8("法师");
    }
    if (bond == "pastor") {
        return QString::fromUtf8("牧师");
    }
    if (bond == "assassin") {
        return QString::fromUtf8("刺客");
    }
    if (bond == "beast") {
        return QString::fromUtf8("野兽");
    }
    if (bond == "chess") {
        return QString::fromUtf8("棋子");
    }
    if (bond == "animal") {
        return QString::fromUtf8("动物");
    }
    if (bond == "building") {
        return QString::fromUtf8("建筑");
    }
    return QString();
}

static QString bondsDisplayText(const characterfactory::CharacterTemplateInfo& info) {
    QStringList labels;
    for (const std::string& bond : info.bonds) {
        const QString label = bondDisplayName(bond);
        if (!label.isEmpty() && !labels.contains(label)) {
            labels.append(label);
        }
    }
    return labels.join(QString::fromUtf8(" / "));
}

CardPanel::CardPanel(QWidget* parent)
    : QFrame(parent)
    , titleLabel(new QLabel(this))
    , expLabel(new QLabel(this))
    , contentBackdrop(new QFrame(this))
    , expTrack(new QFrame(this))
    , expFill(new QFrame(this))
    , cardsLayout(new QGridLayout())
    , closeButton(new QPushButton("X", this)) {
    setObjectName("cardPanel");
    setAttribute(Qt::WA_StyledBackground, true);
    setVisible(false);

    contentBackdrop->setObjectName("contentBackdrop");
    contentBackdrop->setAttribute(Qt::WA_TransparentForMouseEvents);
    contentBackdrop->setStyleSheet("#contentBackdrop { background-color: #efd39a; border: 2px solid rgba(86, 48, 13, 170); border-radius: 4px; }");
    contentBackdrop->lower();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 18, 28, 24);
    root->setSpacing(12);

    auto* top = new QHBoxLayout();
    titleLabel->setStyleSheet("font-size: 24px; font-weight: 900; color: #1e1308; background: transparent;");
    expLabel->setStyleSheet("font-size: 15px; font-weight: 800; color: #281706; background: transparent;");
    closeButton->setFixedSize(34, 30);
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setStyleSheet(
        "QPushButton { background: rgba(64,35,12,190); color: #fff4dc; border: 1px solid #d1a45c; border-radius: 4px; font-weight: 900; }"
        "QPushButton:hover { background: rgba(94,55,20,220); }");
    top->addWidget(titleLabel);
    top->addStretch();
    top->addWidget(expLabel);
    top->addWidget(closeButton);
    root->addLayout(top);

    expTrack->setFixedHeight(48);
    expTrack->setObjectName("expTrack");
    expFill->setParent(expTrack);
    expFill->setObjectName("expFill");
    expTrack->setStyleSheet(R"(
        #expTrack {
            background-color: #5b3713;
            border: 2px solid #f0ca73;
            border-radius: 14px;
        }
        #expFill {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #52d29d, stop:0.5 #f7d96a, stop:1 #ffb13b);
            border: 1px solid rgba(255, 255, 210, 190);
            border-radius: 10px;
        }
    )");
    root->addWidget(expTrack);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setAutoFillBackground(true);
    scroll->viewport()->setAutoFillBackground(true);
    QPalette scrollPalette = scroll->viewport()->palette();
    scrollPalette.setColor(QPalette::Window, QColor(239, 211, 154));
    scroll->viewport()->setPalette(scrollPalette);
    scroll->setStyleSheet("QScrollArea { background: #efd39a; border: none; } QScrollArea > QWidget { background: #efd39a; }");
    auto* cardsHost = new QWidget(scroll);
    cardsHost->setAutoFillBackground(true);
    cardsHost->setStyleSheet("background-color: #efd39a;");
    cardsLayout->setContentsMargins(0, 4, 0, 0);
    cardsLayout->setSpacing(14);
    cardsHost->setLayout(cardsLayout);
    scroll->setWidget(cardsHost);
    root->addWidget(scroll, 1);

    connect(closeButton, &QPushButton::clicked, this, &CardPanel::closePanel);

    setStyleSheet(QString(R"(
        #cardPanel {
            border-image: url("%1") 36 36 36 36 stretch stretch;
            background-color: #efd9ad;
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
    dragSourceIndex = -1;
    rebuild();
    relayoutAnimated(true);
}

void CardPanel::showBag() {
    currentMode = Mode::Bag;
    dragSourceIndex = -1;
    rebuild();
    relayoutAnimated(true);
}

void CardPanel::showTraining() {
    currentMode = Mode::Training;
    dragSourceIndex = -1;
    rebuild();
    relayoutAnimated(true);
}

void CardPanel::closePanel() {
    if (currentMode == Mode::None && !isVisible()) {
        return;
    }
    currentMode = Mode::None;
    dragSourceIndex = -1;
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
        updateExpFill();
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
    connect(animation, &QPropertyAnimation::finished, this, [this, animation]() {
        updateExpFill();
        animation->deleteLater();
    });
    animation->start();
}

bool CardPanel::eventFilter(QObject* watched, QEvent* event) {
    auto* card = qobject_cast<QWidget*>(watched);
    if (!card) {
        return QFrame::eventFilter(watched, event);
    }
    const bool bagCard = card->property("bagCard").toBool();
    const bool trainingCard = card->property("trainingCard").toBool();
    if (!bagCard && !trainingCard) {
        return QFrame::eventFilter(watched, event);
    }

    if (trainingCard) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton) {
                const int ownedCardIndex = card->property("trainingIndex").toInt();
                card->setEnabled(false);
                QTimer::singleShot(0, this, [this, ownedCardIndex]() {
                    emit trainRequested(ownedCardIndex);
                });
                return true;
            }
        }
        return QFrame::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton) {
            dragStartPosition = mouse->pos();
            dragSourceIndex = card->property("bagIndex").toInt();
        }
        return false;
    }

    if (event->type() == QEvent::MouseMove) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        if (!(mouse->buttons() & Qt::LeftButton) || dragSourceIndex < 0) {
            return false;
        }
        if ((mouse->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
            return false;
        }

        auto* mime = new QMimeData();
        mime->setData("application/x-synera-card-index", QByteArray::number(dragSourceIndex));
        auto* drag = new QDrag(card);
        drag->setMimeData(mime);
        drag->setPixmap(card->grab().scaled(card->size() * 0.72, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        drag->setHotSpot(QPoint(drag->pixmap().width() / 2, drag->pixmap().height() / 2));
        drag->exec(Qt::MoveAction);
        dragSourceIndex = -1;
        return true;
    }

    if (event->type() == QEvent::DragEnter) {
        auto* dragEvent = static_cast<QDragEnterEvent*>(event);
        if (dragEvent->mimeData()->hasFormat("application/x-synera-card-index")) {
            dragEvent->acceptProposedAction();
        }
        return true;
    }

    if (event->type() == QEvent::Drop) {
        auto* dropEvent = static_cast<QDropEvent*>(event);
        const int firstIndex = dropEvent->mimeData()->data("application/x-synera-card-index").toInt();
        const int secondIndex = card->property("bagIndex").toInt();
        const bool validIndexes = firstIndex != secondIndex &&
                                  firstIndex >= 0 &&
                                  secondIndex >= 0 &&
                                  firstIndex < static_cast<int>(currentState.ownedCharacterCards.size()) &&
                                  secondIndex < static_cast<int>(currentState.ownedCharacterCards.size());
        if (validIndexes) {
            const OwnedCharacterCard& first = currentState.ownedCharacterCards[static_cast<size_t>(firstIndex)];
            const OwnedCharacterCard& second = currentState.ownedCharacterCards[static_cast<size_t>(secondIndex)];
            if (first.templateId == second.templateId && first.starLevel == second.starLevel && first.starLevel < 3) {
                const QPoint center = card->mapTo(this, card->rect().center());
                playEnhancedMergeEffect(center);
                QTimer::singleShot(160, this, [this, firstIndex, secondIndex]() {
                    emit mergeRequested(firstIndex, secondIndex);
                });
                dropEvent->acceptProposedAction();
                return true;
            }
        }
        dropEvent->ignore();
        return true;
    }

    return QFrame::eventFilter(watched, event);
}

void CardPanel::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    if (contentBackdrop) {
        contentBackdrop->setGeometry(22, 16, std::max(0, width() - 44), std::max(0, height() - 38));
        contentBackdrop->lower();
    }
    updateExpFill();
}

void CardPanel::rebuild() {
    while (QLayoutItem* item = cardsLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->removeEventFilter(this);
            delete widget;
        }
        delete item;
    }
    if (currentMode == Mode::Shop) {
        rebuildShop();
    } else if (currentMode == Mode::Bag) {
        rebuildBag();
    } else if (currentMode == Mode::Training) {
        rebuildTraining();
    }
}

void CardPanel::rebuildShop() {
    titleLabel->setText(QString::fromUtf8("商店"));
    expTrack->setVisible(true);
    const int threshold = shopExpThreshold();
    expLabel->setText(threshold > 0
                          ? QString::fromUtf8("等级 %1  %2/%3").arg(currentState.shop.level).arg(currentState.shop.exp).arg(threshold)
                          : QString::fromUtf8("等级 %1  MAX").arg(currentState.shop.level));
    updateExpFill();

    titleLabel->setText(QString::fromUtf8("商店"));
    expLabel->setText(threshold > 0
                          ? QString::fromUtf8("等级 %1  %2/%3    %4").arg(currentState.shop.level).arg(currentState.shop.exp).arg(threshold).arg(shopProbabilityText())
                          : QString::fromUtf8("等级 %1  MAX    %2").arg(currentState.shop.level).arg(shopProbabilityText()));
    expTrack->setToolTip(shopExperienceTooltip());
    expLabel->setToolTip(shopExperienceTooltip());

    for (int i = 0; i < static_cast<int>(currentState.shop.offers.size()); ++i) {
        const ShopOffer& offer = currentState.shop.offers[static_cast<size_t>(i)];
        const auto info = characterfactory::infoFor(offer.templateId);
        const int cost = info ? costForRarity(info->rarity) : 0;
        cardsLayout->addWidget(createCharacterCard(offer.templateId, 1, cost, offer.sold, i, false), 0, i);
    }
}

void CardPanel::rebuildBag() {
    titleLabel->setText(QString::fromUtf8("背包"));
    expTrack->setVisible(false);
    expLabel->setText(QString::fromUtf8("持有 %1 张，拖动同名同星卡合成").arg(currentState.ownedCharacterCards.size()));

    for (int i = 0; i < static_cast<int>(currentState.ownedCharacterCards.size()); ++i) {
        const OwnedCharacterCard& card = currentState.ownedCharacterCards[static_cast<size_t>(i)];
        cardsLayout->addWidget(createCharacterCard(card.templateId, card.starLevel, 0, false, i, true), i / 4, i % 4);
    }
}

void CardPanel::rebuildTraining() {
    titleLabel->setText(QString::fromUtf8("锻炼"));
    expTrack->setVisible(false);
    expLabel->setText(QString::fromUtf8("选择一张未到达三星的卡，升一星"));

    int column = 0;
    for (int i = 0; i < static_cast<int>(currentState.ownedCharacterCards.size()); ++i) {
        const OwnedCharacterCard& card = currentState.ownedCharacterCards[static_cast<size_t>(i)];
        if (card.starLevel >= 3) {
            continue;
        }
        cardsLayout->addWidget(createCharacterCard(card.templateId, card.starLevel, 0, false, i, false, true),
                               column / 4, column % 4);
        ++column;
    }
}

QWidget* CardPanel::createCharacterCard(int templateId, int starLevel, int cost, bool sold, int index, bool bagCard, bool trainingCard) {
    auto* frame = new QFrame(this);
    frame->setFixedSize(232, (bagCard || trainingCard) ? 404 : 438);
    frame->setObjectName("characterCard");
    frame->setProperty("bagCard", bagCard);
    frame->setProperty("bagIndex", index);
    frame->setProperty("trainingCard", trainingCard);
    frame->setProperty("trainingIndex", index);
    frame->setProperty("templateId", templateId);
    frame->setProperty("starLevel", starLevel);
    frame->setAcceptDrops(bagCard && starLevel < 3);
    frame->setCursor(trainingCard ? Qt::PointingHandCursor
                                  : (bagCard && starLevel < 3 ? Qt::OpenHandCursor : Qt::ArrowCursor));
    frame->setStyleSheet(QString(R"(
        #characterCard {
            border-image: url("%1") 28 28 28 28 stretch stretch;
            background-color: #efd39a;
            border-radius: 6px;
        }
        #characterCard:hover {
            background-color: #f5dda8;
        }
        QLabel {
            background: transparent;
            color: #1b0f05;
        }
    )").arg(uiAssetPath("card_parchment.png")));

    if (bagCard || trainingCard) {
        frame->installEventFilter(this);
    }

    if (sold) {
        frame->setEnabled(false);
        auto* soldOpacity = new QGraphicsOpacityEffect(frame);
        soldOpacity->setOpacity(0.0);
        frame->setGraphicsEffect(soldOpacity);
        return frame;
    }

    const auto info = characterfactory::infoFor(templateId);
    const QString displayName = info ? QString::fromStdString(info->displayName) : QString::fromUtf8("未知角色");
    const QString magicName = info ? QString::fromStdString(info->magicName) : QString();
    const QString skillDescription = info ? QString::fromStdString(info->skillDescription) : QString();
    const QString bonds = info ? bondsDisplayText(*info) : QString();
    const int maxHp = info ? info->maxHp : 0;
    const int maxMp = info ? info->maxMp : 0;
    const int attack = info ? info->attack : 0;
    const int defense = info ? info->defense : 0;
    const int range = info ? info->range : 1;

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(12, 10, 12, 12);
    layout->setSpacing(6);

    auto* art = new QLabel(frame);
    art->setFixedSize(208, (bagCard || trainingCard) ? 202 : 226);
    art->setAlignment(Qt::AlignCenter);
    art->setStyleSheet("background: #111; border: 2px solid rgba(59,32,9,210); border-radius: 6px;");
    if (info) {
        QPixmap pixmap(characterCardPath(info->name));
        if (!pixmap.isNull()) {
            art->setPixmap(pixmap.scaled(art->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
    }
    layout->addWidget(art, 0, Qt::AlignCenter);

    auto* textPanel = new QFrame(frame);
    textPanel->setObjectName("cardTextPanel");
    textPanel->setStyleSheet("#cardTextPanel { background: rgba(255, 235, 190, 235); border: 1px solid rgba(95,54,14,160); border-radius: 6px; }");
    auto* textLayout = new QVBoxLayout(textPanel);
    textLayout->setContentsMargins(8, 6, 8, 6);
    textLayout->setSpacing(3);

    auto* stars = new QLabel(starsForLevel(starLevel), textPanel);
    stars->setAlignment(Qt::AlignCenter);
    stars->setToolTip(QString::fromUtf8("每次升星使最大生命、攻击、防御 +20%，最高三星"));
    stars->setStyleSheet("font-size: 16px; font-weight: 900; color: #a05b00;");
    textLayout->addWidget(stars);

    auto* name = new QLabel(displayName, textPanel);
    name->setAlignment(Qt::AlignCenter);
    name->setWordWrap(true);
    name->setStyleSheet("font-size: 18px; font-weight: 900; color: #1b0f05;");
    textLayout->addWidget(name);

    const double multiplier = 1.0 + 0.2 * (std::max(1, std::min(3, starLevel)) - 1);
    const QString statsText = bonds.isEmpty()
                                  ? QString::fromUtf8("生命 %1  魔力 %2  攻击 %3\n防御 %4  范围 %5")
                                        .arg(std::lround(maxHp * multiplier))
                                        .arg(maxMp)
                                        .arg(std::lround(attack * multiplier))
                                        .arg(std::lround(defense * multiplier))
                                        .arg(range)
                                  : QString::fromUtf8("生命 %1  魔力 %2  攻击 %3\n防御 %4  范围 %5  职业 %6")
                                        .arg(std::lround(maxHp * multiplier))
                                        .arg(maxMp)
                                        .arg(std::lround(attack * multiplier))
                                        .arg(std::lround(defense * multiplier))
                                        .arg(range)
                                        .arg(bonds);
    auto* stats = new QLabel(statsText, textPanel);
    stats->setAlignment(Qt::AlignCenter);
    stats->setWordWrap(true);
    stats->setStyleSheet("font-size: 11px; font-weight: 800; color: #241305;");
    textLayout->addWidget(stats);

    auto* skill = new QLabel(QString::fromUtf8("%1：%2").arg(magicName, skillDescription), textPanel);
    skill->setWordWrap(true);
    skill->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    skill->setStyleSheet("font-size: 10px; font-weight: 700; color: #241305;");
    textLayout->addWidget(skill, 1);

    layout->addWidget(textPanel, 1);

    if (!bagCard && !trainingCard) {
        auto* button = new QPushButton(QString::number(cost), frame);
        button->setIcon(QIcon(uiAssetPath("ui_coin.png")));
        button->setIconSize(QSize(24, 24));
        button->setEnabled(templateId > 0 && currentState.gold >= cost);
        button->setStyleSheet("QPushButton { padding: 6px; background: rgba(73,38,12,235); color: #fff5db; border-radius: 4px; font-weight: 900; font-size: 20px; } QPushButton:disabled { color: #79624a; background: rgba(91,55,22,90); }");
        layout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, frame, index]() {
            playEnhancedCardFlyToBag(frame, index);
        });
    }

    return frame;
}

void CardPanel::updateExpFill() {
    if (!expTrack || !expFill) {
        return;
    }
    const int threshold = shopExpThreshold();
    const double ratio = threshold > 0 ? std::min(1.0, currentState.shop.exp / static_cast<double>(threshold)) : 1.0;
    const int width = std::max(0, static_cast<int>((expTrack->width() - 8) * ratio));
    expFill->setGeometry(4, 4, width, std::max(1, expTrack->height() - 8));
}

void CardPanel::playMergeEffect(const QPoint& center) {
    auto* burst = new QParallelAnimationGroup(this);
    for (int i = 0; i < 12; ++i) {
        auto* star = new QLabel(QString::fromUtf8(i % 3 == 0 ? "★" : "✦"), this);
        star->setAttribute(Qt::WA_TransparentForMouseEvents);
        star->setAlignment(Qt::AlignCenter);
        star->setStyleSheet("color: #ffcf3a; font-size: 20px; font-weight: 900; background: transparent;");
        star->setGeometry(center.x() - 12, center.y() - 12, 24, 24);
        star->show();
        star->raise();

        auto* opacity = new QGraphicsOpacityEffect(star);
        star->setGraphicsEffect(opacity);
        auto* move = new QPropertyAnimation(star, "pos", burst);
        const int dx = QRandomGenerator::global()->bounded(-95, 96);
        const int dy = QRandomGenerator::global()->bounded(-85, 56);
        move->setDuration(520);
        move->setStartValue(star->pos());
        move->setEndValue(star->pos() + QPoint(dx, dy));
        move->setEasingCurve(QEasingCurve::OutCubic);

        auto* fade = new QPropertyAnimation(opacity, "opacity", burst);
        fade->setDuration(520);
        fade->setStartValue(1.0);
        fade->setEndValue(0.0);
        burst->addAnimation(move);
        burst->addAnimation(fade);
        connect(burst, &QParallelAnimationGroup::finished, star, &QLabel::deleteLater);
    }
    connect(burst, &QParallelAnimationGroup::finished, burst, &QParallelAnimationGroup::deleteLater);
    burst->start();
}

void CardPanel::playCardFlyToBag(QWidget* sourceCard, int offerIndex) {
    if (!sourceCard) {
        emit buyRequested(offerIndex);
        return;
    }

    const QRect startRect(mapFromGlobal(sourceCard->mapToGlobal(QPoint(0, 0))), sourceCard->size());
    const QPoint endCenter(width() - 72, 36);
    auto* flyer = new QLabel(this);
    flyer->setAttribute(Qt::WA_TransparentForMouseEvents);
    flyer->setPixmap(sourceCard->grab().scaled(sourceCard->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    flyer->setScaledContents(true);
    flyer->setGeometry(startRect);
    flyer->show();
    flyer->raise();
    sourceCard->hide();

    auto* opacity = new QGraphicsOpacityEffect(flyer);
    flyer->setGraphicsEffect(opacity);

    auto* group = new QParallelAnimationGroup(this);
    auto* move = new QPropertyAnimation(flyer, "geometry", group);
    move->setDuration(420);
    move->setStartValue(startRect);
    move->setEndValue(QRect(endCenter.x() - 24, endCenter.y() - 34, 48, 68));
    move->setEasingCurve(QEasingCurve::InOutCubic);
    auto* fade = new QPropertyAnimation(opacity, "opacity", group);
    fade->setDuration(420);
    fade->setStartValue(1.0);
    fade->setEndValue(0.18);
    group->addAnimation(move);
    group->addAnimation(fade);

    for (int i = 0; i < 6; ++i) {
        const double t = i / 5.0;
        const QPoint point(startRect.center().x() + static_cast<int>((endCenter.x() - startRect.center().x()) * t),
                           startRect.center().y() + static_cast<int>((endCenter.y() - startRect.center().y()) * t));
        playTrailParticle(point, i * 45);
    }

    connect(group, &QParallelAnimationGroup::finished, this, [this, flyer, group, offerIndex]() {
        flyer->deleteLater();
        group->deleteLater();
        emit buyRequested(offerIndex);
    });
    group->start();
}

void CardPanel::playTrailParticle(const QPoint& point, int delayMs) {
    QTimer::singleShot(delayMs, this, [this, point]() {
        auto* dot = new QLabel(QString::fromUtf8("✦"), this);
        dot->setAttribute(Qt::WA_TransparentForMouseEvents);
        dot->setAlignment(Qt::AlignCenter);
        dot->setStyleSheet("color: #ffe38a; font-size: 16px; font-weight: 900; background: transparent;");
        dot->setGeometry(point.x() - 8, point.y() - 8, 16, 16);
        dot->show();
        dot->raise();

        auto* opacity = new QGraphicsOpacityEffect(dot);
        dot->setGraphicsEffect(opacity);
        auto* group = new QParallelAnimationGroup(dot);
        auto* move = new QPropertyAnimation(dot, "pos", group);
        move->setDuration(360);
        move->setStartValue(dot->pos());
        move->setEndValue(dot->pos() + QPoint(QRandomGenerator::global()->bounded(-16, 17),
                                              QRandomGenerator::global()->bounded(-24, 8)));
        auto* fade = new QPropertyAnimation(opacity, "opacity", group);
        fade->setDuration(360);
        fade->setStartValue(0.9);
        fade->setEndValue(0.0);
        group->addAnimation(move);
        group->addAnimation(fade);
        connect(group, &QParallelAnimationGroup::finished, dot, &QLabel::deleteLater);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void CardPanel::playEnhancedMergeEffect(const QPoint& center) {
    auto* flash = new QLabel(QString::fromUtf8("★"), this);
    flash->setAttribute(Qt::WA_TransparentForMouseEvents);
    flash->setAlignment(Qt::AlignCenter);
    flash->setStyleSheet("color: #fff1a8; font-size: 76px; font-weight: 900; background: transparent;");
    flash->setGeometry(center.x() - 44, center.y() - 48, 88, 88);
    flash->show();
    flash->raise();

    auto* flashOpacity = new QGraphicsOpacityEffect(flash);
    flash->setGraphicsEffect(flashOpacity);
    auto* flashGroup = new QParallelAnimationGroup(flash);
    auto* flashGrow = new QPropertyAnimation(flash, "geometry", flashGroup);
    flashGrow->setDuration(560);
    flashGrow->setStartValue(QRect(center.x() - 44, center.y() - 48, 88, 88));
    flashGrow->setEndValue(QRect(center.x() - 86, center.y() - 92, 172, 172));
    flashGrow->setEasingCurve(QEasingCurve::OutBack);
    auto* flashFade = new QPropertyAnimation(flashOpacity, "opacity", flashGroup);
    flashFade->setDuration(660);
    flashFade->setStartValue(1.0);
    flashFade->setEndValue(0.0);
    flashGroup->addAnimation(flashGrow);
    flashGroup->addAnimation(flashFade);
    connect(flashGroup, &QParallelAnimationGroup::finished, flash, &QLabel::deleteLater);
    flashGroup->start(QAbstractAnimation::DeleteWhenStopped);

    for (int i = 0; i < 26; ++i) {
        auto* star = new QLabel(QString::fromUtf8(i % 3 == 0 ? "★" : "✦"), this);
        star->setAttribute(Qt::WA_TransparentForMouseEvents);
        star->setAlignment(Qt::AlignCenter);
        star->setStyleSheet("color: #ffd23d; font-size: 24px; font-weight: 900; background: transparent;");
        star->setGeometry(center.x() - 14, center.y() - 14, 28, 28);
        star->show();
        star->raise();

        auto* opacity = new QGraphicsOpacityEffect(star);
        star->setGraphicsEffect(opacity);
        auto* group = new QParallelAnimationGroup(star);
        auto* move = new QPropertyAnimation(star, "pos", group);
        move->setDuration(780);
        move->setStartValue(star->pos());
        move->setEndValue(star->pos() + QPoint(QRandomGenerator::global()->bounded(-175, 176),
                                               QRandomGenerator::global()->bounded(-155, 116)));
        move->setEasingCurve(QEasingCurve::OutCubic);
        auto* fade = new QPropertyAnimation(opacity, "opacity", group);
        fade->setDuration(780);
        fade->setStartValue(1.0);
        fade->setEndValue(0.0);
        group->addAnimation(move);
        group->addAnimation(fade);
        connect(group, &QParallelAnimationGroup::finished, star, &QLabel::deleteLater);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void CardPanel::playEnhancedCardFlyToBag(QWidget* sourceCard, int offerIndex) {
    if (!sourceCard) {
        emit buyRequested(offerIndex);
        return;
    }

    const QRect startRect(mapFromGlobal(sourceCard->mapToGlobal(QPoint(0, 0))), sourceCard->size());
    const QPoint startCenter = startRect.center();
    const QPoint endCenter(width() - 72, 34);
    const QPoint control((startCenter.x() + endCenter.x()) / 2, std::min(startCenter.y(), endCenter.y()) - 170);
    const QSize flyerStartSize(std::max(46, static_cast<int>(startRect.width() * 0.62)),
                               std::max(64, static_cast<int>(startRect.height() * 0.62)));
    const QRect flyerStartRect(startCenter.x() - flyerStartSize.width() / 2,
                               startCenter.y() - flyerStartSize.height() / 2 - 18,
                               flyerStartSize.width(),
                               flyerStartSize.height());

    auto* flyer = new QLabel(this);
    flyer->setAttribute(Qt::WA_TransparentForMouseEvents);
    flyer->setPixmap(sourceCard->grab().scaled(sourceCard->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    flyer->setScaledContents(true);
    sourceCard->setEnabled(false);
    auto* sourceOpacity = new QGraphicsOpacityEffect(sourceCard);
    sourceOpacity->setOpacity(0.0);
    sourceCard->setGraphicsEffect(sourceOpacity);
    if (sourceCard->parentWidget()) {
        sourceCard->parentWidget()->update();
    }
    update();
    flyer->setGeometry(flyerStartRect);
    flyer->show();
    flyer->raise();

    auto* opacity = new QGraphicsOpacityEffect(flyer);
    flyer->setGraphicsEffect(opacity);

    auto* flight = new QVariantAnimation(this);
    flight->setDuration(760);
    flight->setStartValue(0.0);
    flight->setEndValue(1.0);
    flight->setEasingCurve(QEasingCurve::InOutCubic);
    connect(flight, &QVariantAnimation::valueChanged, this, [this, flyer, opacity, startRect, startCenter, control, endCenter](const QVariant& value) {
        const double t = value.toDouble();
        const double inv = 1.0 - t;
        const QPointF point = inv * inv * QPointF(startCenter) + 2.0 * inv * t * QPointF(control) + t * t * QPointF(endCenter);
        const double scale = 0.62 - 0.34 * t;
        const QSize size(std::max(46, static_cast<int>(startRect.width() * scale)),
                         std::max(64, static_cast<int>(startRect.height() * scale)));
        flyer->setGeometry(static_cast<int>(point.x()) - size.width() / 2,
                           static_cast<int>(point.y()) - size.height() / 2,
                           size.width(),
                           size.height());
        opacity->setOpacity(1.0 - 0.45 * t);
        if (QRandomGenerator::global()->bounded(100) < 40) {
            playEnhancedTrailParticle(QPoint(static_cast<int>(point.x()), static_cast<int>(point.y())), 0, 18, "#ffe68a");
        }
    });

    for (int i = 0; i < 14; ++i) {
        const double t = i / 13.0;
        const double inv = 1.0 - t;
        const QPointF point = inv * inv * QPointF(startCenter) + 2.0 * inv * t * QPointF(control) + t * t * QPointF(endCenter);
        playEnhancedTrailParticle(QPoint(static_cast<int>(point.x()), static_cast<int>(point.y())), i * 42, 16, "#ffd96b");
    }

    connect(flight, &QVariantAnimation::finished, this, [this, flyer, flight, endCenter]() {
        flyer->deleteLater();
        flight->deleteLater();
        playEnhancedMergeEffect(endCenter);
    });
    flight->start();
    emit buyRequested(offerIndex);
}

void CardPanel::playEnhancedTrailParticle(const QPoint& point, int delayMs, int size, const QString& color) {
    QTimer::singleShot(delayMs, this, [this, point, size, color]() {
        auto* dot = new QLabel(QString::fromUtf8("✦"), this);
        dot->setAttribute(Qt::WA_TransparentForMouseEvents);
        dot->setAlignment(Qt::AlignCenter);
        dot->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: 900; background: transparent;").arg(color).arg(size));
        dot->setGeometry(point.x() - size / 2, point.y() - size / 2, size, size);
        dot->show();
        dot->raise();

        auto* opacity = new QGraphicsOpacityEffect(dot);
        dot->setGraphicsEffect(opacity);
        auto* group = new QParallelAnimationGroup(dot);
        auto* move = new QPropertyAnimation(dot, "pos", group);
        move->setDuration(520);
        move->setStartValue(dot->pos());
        move->setEndValue(dot->pos() + QPoint(QRandomGenerator::global()->bounded(-26, 27),
                                              QRandomGenerator::global()->bounded(-38, 14)));
        move->setEasingCurve(QEasingCurve::OutCubic);
        auto* fade = new QPropertyAnimation(opacity, "opacity", group);
        fade->setDuration(520);
        fade->setStartValue(1.0);
        fade->setEndValue(0.0);
        group->addAnimation(move);
        group->addAnimation(fade);
        connect(group, &QParallelAnimationGroup::finished, dot, &QLabel::deleteLater);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    });
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

QString CardPanel::shopProbabilityText() const {
    if (currentState.shop.level == 1) {
        return QString::fromUtf8("概率 1/2/3/4: 75% / 25% / 0% / 0%");
    }
    if (currentState.shop.level == 2) {
        return QString::fromUtf8("概率 1/2/3/4: 50% / 30% / 20% / 0%");
    }
    if (currentState.shop.level == 3) {
        return QString::fromUtf8("概率 1/2/3/4: 50% / 20% / 20% / 10%");
    }
    return QString::fromUtf8("概率 1/2/3/4: 20% / 20% / 30% / 30%");
}

QString CardPanel::shopExperienceTooltip() const {
    return QString::fromUtf8(
        "商店经验获取：\n"
        "购买任意卡牌 +1 经验；每次战斗胜利 +1 经验。\n\n"
        "升级需求：1→2 需要10经验，2→3 需要15经验，3→4 需要20经验。\n\n"
        "稀有度出现概率：\n"
        "1级：75% / 25% / 0% / 0%\n"
        "2级：50% / 30% / 20% / 0%\n"
        "3级：50% / 20% / 20% / 10%\n"
        "4级：20% / 20% / 30% / 30%");
}
