#include "eventpage.h"
#include "app/eventrules.h"
#include "entity/character/characterfactory.h"
#include "gui/widgets/gamehud.h"
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <functional>

class HexTechCardFrame : public QFrame {
public:
    explicit HexTechCardFrame(QWidget* parent = nullptr)
        : QFrame(parent) {}

    std::function<void()> onClicked;

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && onClicked) {
            onClicked();
        }
        QFrame::mousePressEvent(event);
    }
};

class RestCardFrame : public QFrame {
public:
    explicit RestCardFrame(QWidget* parent = nullptr)
        : QFrame(parent) {}

    std::function<void()> onClicked;
    std::function<void()> onHovered;
    std::function<void()> onLeft;

protected:
    void enterEvent(QEnterEvent* event) override {
        if (onHovered) {
            onHovered();
        }
        QFrame::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        if (onLeft) {
            onLeft();
        }
        QFrame::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && isEnabled() && onClicked) {
            const std::function<void()> callback = onClicked;
            QTimer::singleShot(0, this, [callback]() {
                if (callback) {
                    callback();
                }
            });
            event->accept();
            return;
        }
        QFrame::mousePressEvent(event);
    }
};

class NormalOptionButton : public QPushButton {
public:
    explicit NormalOptionButton(QWidget* parent = nullptr)
        : QPushButton(parent) {}

    std::function<void()> onHovered;
    std::function<void()> onLeft;

protected:
    void enterEvent(QEnterEvent* event) override {
        if (onHovered) {
            onHovered();
        }
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        if (onLeft) {
            onLeft();
        }
        QPushButton::leaveEvent(event);
    }
};

class NormalEventView : public QWidget {
public:
    explicit NormalEventView(QWidget* parent = nullptr)
        : QWidget(parent)
        , titleLabel(new QLabel(this))
        , bodyLabel(new QLabel(this))
        , descriptionLabel(new QLabel(this))
        , optionsLayout(new QVBoxLayout()) {
        setAttribute(Qt::WA_TranslucentBackground);
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(12);
        layout->setAlignment(Qt::AlignRight | Qt::AlignTop);

        titleLabel->setFixedWidth(470);
        titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        titleLabel->setStyleSheet(R"(
            font-size: 28px;
            font-weight: 900;
            color: #fff5cf;
            background-color: rgba(14, 15, 17, 185);
            border: 2px solid rgba(239, 211, 128, 175);
            border-radius: 8px;
            padding: 12px 18px;
        )");
        layout->addWidget(titleLabel, 0, Qt::AlignRight);

        bodyLabel->setFixedWidth(470);
        bodyLabel->setMinimumHeight(150);
        bodyLabel->setWordWrap(true);
        bodyLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        bodyLabel->setStyleSheet(R"(
            font-size: 18px;
            color: #fff9e6;
            background-color: rgba(14, 15, 17, 185);
            border: 2px solid rgba(239, 211, 128, 175);
            border-radius: 8px;
            padding: 18px;
        )");
        layout->addWidget(bodyLabel, 0, Qt::AlignRight);

        auto* optionsWidget = new QWidget(this);
        optionsWidget->setAttribute(Qt::WA_TranslucentBackground);
        optionsWidget->setFixedWidth(470);
        optionsLayout->setContentsMargins(0, 0, 0, 0);
        optionsLayout->setAlignment(Qt::AlignRight | Qt::AlignTop);
        optionsWidget->setLayout(optionsLayout);
        layout->addWidget(optionsWidget, 0, Qt::AlignRight);

        descriptionLabel->setFixedWidth(470);
        descriptionLabel->setWordWrap(true);
        descriptionLabel->setMinimumHeight(44);
        descriptionLabel->setStyleSheet(R"(
            font-size: 18px;
            font-weight: 800;
            color: #ffb946;
            background-color: rgba(14, 15, 17, 150);
            border: 1px solid rgba(239, 211, 128, 100);
            border-radius: 8px;
            padding: 8px 14px;
        )");
        layout->addWidget(descriptionLabel, 0, Qt::AlignRight);
        layout->addStretch();
    }

    std::function<void(int)> onOptionSelected;

    void setState(const GameState& state) {
        currentState = state;
        titleLabel->setText(QString::fromStdString(state.currentEvent.title));
        bodyLabel->setText(QString::fromStdString(state.currentEvent.text));
        descriptionLabel->setText(QString::fromUtf8("选择一个选项"));

        while (QLayoutItem* item = optionsLayout->takeAt(0)) {
            if (QWidget* widget = item->widget()) {
                delete widget;
            }
            delete item;
        }

        const int optionCount = static_cast<int>(state.currentEvent.options.size());
        optionsLayout->setSpacing(optionCount == 4 ? 12 : (optionCount == 2 ? 20 : 16));
        for (int i = 0; i < optionCount; ++i) {
            const EventOption& option = state.currentEvent.options[static_cast<size_t>(i)];
            auto* button = new NormalOptionButton(this);
            button->setText(QString::fromStdString(option.label));
            button->setMinimumSize(420, optionCount == 4 ? 54 : 64);
            button->setCursor(Qt::PointingHandCursor);
            const std::string disabledReason = disabledReasonForEventOption(currentState, option);
            button->setToolTip(QString::fromStdString(disabledReason.empty() ? option.description : disabledReason));
            button->setProperty("blocked", !disabledReason.empty());
            button->setStyleSheet(R"(
                QPushButton {
                    background-color: rgba(25, 27, 30, 190);
                    border: 2px solid rgba(239, 211, 128, 190);
                    border-radius: 8px;
                    color: #fff4cf;
                    font-size: 20px;
                    font-weight: 800;
                    text-align: left;
                    padding-left: 24px;
                }
                QPushButton:hover {
                    background-color: rgba(55, 43, 24, 220);
                    border: 3px solid #ffb946;
                }
                QPushButton[blocked="true"] {
                    color: rgba(255, 244, 207, 95);
                    background-color: rgba(20, 20, 20, 120);
                    border: 2px solid rgba(130, 120, 100, 100);
                }
            )");
            button->onHovered = [this, option, disabledReason]() {
                if (!disabledReason.empty()) {
                    descriptionLabel->setText(QString::fromStdString(disabledReason));
                } else if (!option.description.empty()) {
                    descriptionLabel->setText(QString::fromStdString(option.description));
                } else {
                    descriptionLabel->setText(QString::fromStdString(option.label));
                }
            };
            button->onLeft = [this]() {
                descriptionLabel->setText(QString::fromUtf8("选择一个选项"));
            };
            optionsLayout->addWidget(button, 0, Qt::AlignRight);
            QObject::connect(button, &QPushButton::clicked, this, [this, button, i]() {
                if (button->property("blocked").toBool()) {
                    return;
                }
                if (onOptionSelected) {
                    onOptionSelected(i);
                }
            });
        }
    }

private:
    QLabel* titleLabel;
    QLabel* bodyLabel;
    QLabel* descriptionLabel;
    QVBoxLayout* optionsLayout;
    GameState currentState;
};

static QString assetPath(const std::string& relativePath) {
    const QString path = QString::fromStdString(relativePath);
    const QString runtimePath = QDir(QCoreApplication::applicationDirPath()).filePath(path);
    if (QFile::exists(runtimePath)) {
        return runtimePath;
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(path);
}

static QString characterCardPath(const std::string& name) {
    const QString relative = QDir("assets/characters").filePath(QString::fromStdString(name) + "/card.png");
    return QDir(QCoreApplication::applicationDirPath()).filePath(relative);
}

static QString restOptionLabel(RestOption option) {
    switch (option) {
    case RestOption::Rest:
        return QString::fromUtf8("休息");
    case RestOption::Train:
        return QString::fromUtf8("锻炼");
    case RestOption::Dig:
        return QString::fromUtf8("挖宝");
    case RestOption::Forge:
        return QString::fromUtf8("锻造");
    }
    return {};
}

static QString restOptionDescription(RestOption option) {
    switch (option) {
    case RestOption::Rest:
        return QString::fromUtf8("恢复15点生命（注意上限）");
    case RestOption::Train:
        return QString::fromUtf8("选择一张未到达三星的卡，升一星");
    case RestOption::Dig:
        return QString::fromUtf8("获取1-5金币");
    case RestOption::Forge:
        return QString::fromUtf8("获得一件随机装备");
    }
    return {};
}

static std::string restOptionImagePath(RestOption option) {
    switch (option) {
    case RestOption::Rest:
        return "assets/events/rest_action_rest.png";
    case RestOption::Train:
        return "assets/events/rest_action_train.png";
    case RestOption::Dig:
        return "assets/events/rest_action_dig.png";
    case RestOption::Forge:
        return "assets/events/rest_action_forge.png";
    }
    return {};
}

static QPixmap loadScaledPixmap(const QString& path, const QSize& targetSize) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize sourceSize = reader.size();
    if (sourceSize.isValid() && !sourceSize.isEmpty()) {
        reader.setScaledSize(sourceSize.scaled(targetSize, Qt::KeepAspectRatioByExpanding));
    }
    const QImage image = reader.read();
    if (image.isNull()) {
        return {};
    }
    return QPixmap::fromImage(image).scaled(targetSize,
                                            Qt::KeepAspectRatioByExpanding,
                                            Qt::SmoothTransformation);
}

EventPage::EventPage(QWidget* parent)
    : QWidget(parent)
    , hud(new GameHud(this))
    , contentLayout(nullptr)
    , normalEventView(new NormalEventView(this))
    , titleLabel(new QLabel(this))
    , bodyLabel(new QLabel(this))
    , optionsWidget(nullptr)
    , optionsLayout(nullptr)
    , hexTechLayout(nullptr)
    , restOptionsWidget(new QWidget(this))
    , restScrollArea(new QScrollArea(this))
    , restLayout(nullptr)
    , restDescriptionLabel(new QLabel(this))
    , restEquipmentHintLabel(new QLabel(this))
    , confirmHexTechButton(new QPushButton(QString::fromUtf8("确认选择"), this))
    , selectedHexTechChoice(-1) {
    setAutoFillBackground(false);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(hud);

    auto* content = new QWidget(this);
    content->setAttribute(Qt::WA_TranslucentBackground);
    contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(48, 12, 48, 32);
    contentLayout->setSpacing(12);
    normalEventView->setVisible(false);
    contentLayout->addWidget(normalEventView);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 32px; font-weight: 800; color: #ffffff; background: transparent;");
    contentLayout->addWidget(titleLabel);

    bodyLabel->setAlignment(Qt::AlignCenter);
    bodyLabel->setWordWrap(true);
    bodyLabel->setStyleSheet("font-size: 19px; color: #ffffff; background: transparent;");
    contentLayout->addWidget(bodyLabel);

    optionsWidget = new QWidget(this);
    optionsWidget->setAttribute(Qt::WA_TranslucentBackground);
    optionsLayout = new QVBoxLayout(optionsWidget);
    optionsLayout->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(optionsWidget);

    auto* hexTechWidget = new QWidget(this);
    hexTechWidget->setAttribute(Qt::WA_TranslucentBackground);
    auto* hexTechOuterLayout = new QVBoxLayout(hexTechWidget);
    hexTechOuterLayout->setContentsMargins(0, 8, 0, 0);
    hexTechOuterLayout->setSpacing(14);
    hexTechLayout = new QHBoxLayout();
    hexTechLayout->setAlignment(Qt::AlignCenter);
    hexTechLayout->setSpacing(30);
    hexTechOuterLayout->addLayout(hexTechLayout);
    confirmHexTechButton->setEnabled(false);
    confirmHexTechButton->setVisible(false);
    confirmHexTechButton->setMinimumWidth(180);
    hexTechOuterLayout->addWidget(confirmHexTechButton, 0, Qt::AlignCenter);
    contentLayout->addWidget(hexTechWidget);

    restDescriptionLabel->setAlignment(Qt::AlignCenter);
    restDescriptionLabel->setWordWrap(true);
    restDescriptionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    restDescriptionLabel->setFixedHeight(100);
    restDescriptionLabel->setStyleSheet(R"(
        font-size: 34px;
        font-weight: 900;
        color: #ff9d2e;
        background: transparent;
        padding: 12px 0;
    )");
    contentLayout->addWidget(restDescriptionLabel);

    restOptionsWidget->setAttribute(Qt::WA_TranslucentBackground);
    restOptionsWidget->setFixedSize(1260, 340);
    restOptionsWidget->setVisible(false);
    contentLayout->addWidget(restOptionsWidget, 0, Qt::AlignHCenter);

    auto* restWidget = new QWidget(restScrollArea);
    restWidget->setAttribute(Qt::WA_TranslucentBackground);
    restLayout = new QGridLayout(restWidget);
    restLayout->setContentsMargins(0, 0, 0, 0);
    restLayout->setHorizontalSpacing(26);
    restLayout->setVerticalSpacing(18);
    restLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    restScrollArea->setWidget(restWidget);
    restScrollArea->setWidgetResizable(true);
    restScrollArea->setFrameShape(QFrame::NoFrame);
    restScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    restScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    restScrollArea->setStyleSheet(R"(
        QScrollArea {
            background: transparent;
            border: none;
        }
        QScrollArea > QWidget,
        QScrollArea > QWidget > QWidget {
            background: transparent;
        }
    )");
    contentLayout->addWidget(restScrollArea, 0, Qt::AlignHCenter);

    restEquipmentHintLabel->setAlignment(Qt::AlignCenter);
    restEquipmentHintLabel->setWordWrap(true);
    restEquipmentHintLabel->setVisible(false);
    restEquipmentHintLabel->setFixedHeight(58);
    restEquipmentHintLabel->setFixedWidth(940);
    restEquipmentHintLabel->setStyleSheet(R"(
        font-size: 18px;
        font-weight: 800;
        color: #fff4cd;
        background-color: rgba(18, 18, 18, 155);
        border: 2px solid rgba(255, 208, 96, 150);
        border-radius: 8px;
        padding: 8px 18px;
    )");
    restEquipmentHintLabel->setText(QString::fromUtf8("你可以在这里脱下装备和合成装备(拖动合成，蓝色胚子和绿色素材合成高级装备)"));
    contentLayout->addStretch(1);
    contentLayout->addWidget(restEquipmentHintLabel, 0, Qt::AlignHCenter | Qt::AlignBottom);

    layout->addWidget(content, 1);

    connect(hud, &GameHud::saveRequested, this, &EventPage::saveRequested);
    connect(hud, &GameHud::bagRequested, this, &EventPage::bagRequested);
    connect(hud, &GameHud::shopRequested, this, &EventPage::shopRequested);
    connect(hud, &GameHud::equipmentRequested, this, &EventPage::equipmentRequested);
    connect(hud, &GameHud::returnToStartRequested, this, &EventPage::returnToStartRequested);
    connect(confirmHexTechButton, &QPushButton::clicked, this, [this]() {
        if (selectedHexTechChoice >= 0) {
            emit hexTechCardSelected(selectedHexTechChoice);
        }
    });
    normalEventView->onOptionSelected = [this](int optionIndex) {
        emit eventOptionSelected(optionIndex);
    };

    setStyleSheet(R"(
        EventPage {
            background-color: #25272d;
        }
        QPushButton {
            padding: 9px 22px;
            font-size: 16px;
        }
    )");
}

void EventPage::setState(const GameState& state) {
    currentState = state;
    hud->setState(state);
}

void EventPage::setElapsedSeconds(int seconds) {
    hud->setElapsedSeconds(seconds);
}

void EventPage::showEvent(int nodeId) {
    updateBackground();
    if (currentState.currentEvent.active) {
        titleLabel->setText(QString::fromStdString(currentState.currentEvent.title));
        bodyLabel->setText(QString::fromStdString(currentState.currentEvent.text));
    } else {
        titleLabel->setText(QString("Event Node %1").arg(nodeId));
        bodyLabel->setText("No event data.");
    }
    rebuildOptions();
    update();
}

void EventPage::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(37, 39, 45));

    const int topOffset = hud ? hud->height() : 0;
    const QRect targetRect(0, topOffset, width(), height() - topOffset);
    if (!backgroundPixmap.isNull() && !targetRect.isEmpty()) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        const QPixmap scaled = backgroundPixmap.scaled(targetRect.size(),
                                                       Qt::KeepAspectRatioByExpanding,
                                                       Qt::SmoothTransformation);
        const QPoint topLeft(targetRect.left() + (targetRect.width() - scaled.width()) / 2,
                             targetRect.top() + (targetRect.height() - scaled.height()) / 2);
        painter.setOpacity(0.9);
        painter.drawPixmap(topLeft, scaled);
        painter.setOpacity(1.0);
        painter.fillRect(targetRect, QColor(0, 0, 0, 35));
    }

    QWidget::paintEvent(event);
}

void EventPage::rebuildOptions() {
    if (!optionsLayout) {
        return;
    }

    while (QLayoutItem* item = optionsLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            delete widget;
        }
        delete item;
    }

    while (QLayoutItem* item = hexTechLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            delete widget;
        }
        delete item;
    }
    while (QLayoutItem* item = restLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            delete widget;
        }
        delete item;
    }
    for (QWidget* widget : restOptionsWidget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
        delete widget;
    }
    selectedHexTechChoice = -1;
    confirmHexTechButton->setEnabled(false);
    confirmHexTechButton->setVisible(false);
    restDescriptionLabel->setVisible(false);
    restEquipmentHintLabel->setVisible(false);
    restOptionsWidget->setVisible(false);
    restScrollArea->setVisible(false);
    restScrollArea->setMinimumSize(0, 0);
    restScrollArea->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    normalEventView->setVisible(false);

    if (!currentState.currentEvent.active) {
        setSpecialEventLayoutVisible(true);
        return;
    }

    if (currentState.currentEvent.hexTechSelection) {
        setSpecialEventLayoutVisible(true);
        contentLayout->setContentsMargins(48, 8, 48, 24);
        titleLabel->setFixedHeight(58);
        bodyLabel->setFixedSize(1180, 118);
        bodyLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        bodyLabel->setStyleSheet(R"(
            font-size: 18px;
            line-height: 130%;
            color: #ffffff;
            background-color: rgba(22, 23, 25, 115);
            border: 1px solid rgba(255, 230, 170, 95);
            border-radius: 8px;
            padding: 10px 18px;
        )");
        confirmHexTechButton->setVisible(true);
        for (int i = 0; i < static_cast<int>(currentState.currentHexTechChoices.size()); ++i) {
            hexTechLayout->addWidget(createHexTechCard(i));
        }
        return;
    }

    if (currentState.currentEvent.restSelection) {
        setSpecialEventLayoutVisible(true);
        contentLayout->setContentsMargins(8, 8, 8, 16);
        bodyLabel->setVisible(false);
        restEquipmentHintLabel->setVisible(true);
        restDescriptionLabel->setVisible(true);
        restDescriptionLabel->setText(QString::fromUtf8("选择一项行动"));
        restOptionsWidget->setVisible(true);
        const struct {
            RestOption option;
            QPoint pos;
        } placements[] = {
            {RestOption::Rest, QPoint(0, 112)},
            {RestOption::Train, QPoint(320, 20)},
            {RestOption::Dig, QPoint(640, 20)},
            {RestOption::Forge, QPoint(960, 112)},
        };
        for (const auto& placement : placements) {
            QWidget* card = createRestOptionCard(placement.option);
            card->setParent(restOptionsWidget);
            card->move(placement.pos);
            card->show();
        }
        return;
    }

    if (currentState.currentEvent.restTrainingSelection) {
        setSpecialEventLayoutVisible(true);
        restDescriptionLabel->setVisible(true);
        restDescriptionLabel->setText(QString::fromUtf8("选择一张未到达三星的卡，升一星"));
        restScrollArea->setVisible(true);
        auto* button = new QPushButton(QString::fromUtf8("打开锻炼面板"), this);
        button->setMinimumSize(280, 68);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(R"(
            QPushButton {
                background-color: rgba(255, 246, 182, 225);
                border: 3px solid rgba(255, 226, 115, 225);
                border-radius: 8px;
                color: #5a310a;
                font-size: 24px;
                font-weight: 900;
            }
            QPushButton:hover {
                background-color: rgba(255, 252, 213, 245);
                border: 4px solid #ffb946;
            }
        )");
        restLayout->addWidget(button, 0, 0, Qt::AlignTop | Qt::AlignHCenter);
        connect(button, &QPushButton::clicked, this, [this]() {
            QTimer::singleShot(0, this, [this]() {
                emit trainingPanelRequested();
            });
        });
        return;
    }

    if (currentState.currentEvent.ownedCardSelection) {
        setSpecialEventLayoutVisible(true);
        restDescriptionLabel->setVisible(true);
        restDescriptionLabel->setText(QString::fromStdString(currentState.currentEvent.selectionPrompt));
        restScrollArea->setVisible(true);
        int column = 0;
        const std::vector<int> indexes =
            selectableOwnedCardIndexesForEventFilter(currentState, currentState.currentEvent.selectionFilter);
        for (int ownedCardIndex : indexes) {
            restLayout->addWidget(createEventOwnedCard(ownedCardIndex), column / 4, column % 4, Qt::AlignTop);
            ++column;
        }
        return;
    }

    setSpecialEventLayoutVisible(false);
    normalEventView->setVisible(true);
    normalEventView->setState(currentState);
}

QWidget* EventPage::createHexTechCard(int choiceIndex) {
    const HexTechDefinition& choice = currentState.currentHexTechChoices[static_cast<size_t>(choiceIndex)];

    auto* frame = new HexTechCardFrame(this);
    frame->setFixedWidth(300);
    frame->setCursor(Qt::PointingHandCursor);
    frame->setProperty("choiceIndex", choiceIndex);
    frame->setStyleSheet(R"(
        QFrame {
            background-color: rgba(10, 12, 16, 125);
            border: 2px solid rgba(230, 210, 150, 95);
            border-radius: 8px;
        }
        QFrame[selected="true"] {
            border: 2px solid #f0c85a;
            background-color: rgba(36, 29, 16, 150);
        }
    )");

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(12, 12, 12, 14);
    layout->setSpacing(10);

    auto* imageLabel = new QLabel(frame);
    imageLabel->setFixedSize(246, 344);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("background: transparent; border: none;");
    const QPixmap pixmap = loadScaledPixmap(assetPath(choice.cardImagePath), imageLabel->size());
    if (!pixmap.isNull()) {
        imageLabel->setPixmap(pixmap);
    } else {
        imageLabel->setText("HexTech");
    }
    layout->addWidget(imageLabel, 0, Qt::AlignCenter);

    auto* title = new QLabel(QString::fromStdString(choice.title), frame);
    title->setWordWrap(true);
    title->setAlignment(Qt::AlignCenter);
    title->setMinimumWidth(246);
    title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    title->setStyleSheet("font-size: 22px; font-weight: 800; color: #ffffff; border: none; background: transparent;");
    layout->addWidget(title);

    auto* description = new QLabel(QString::fromStdString(choice.description), frame);
    description->setWordWrap(true);
    description->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    description->setMinimumSize(246, 112);
    description->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    description->setStyleSheet("font-size: 16px; line-height: 130%; color: #ffffff; border: none; background: transparent;");
    layout->addWidget(description);

    frame->setProperty("selected", false);
    frame->setToolTip(QString::fromStdString(choice.name));
    frame->setObjectName(QString("hexTechCard_%1").arg(choiceIndex));
    frame->setAttribute(Qt::WA_StyledBackground, true);
    frame->onClicked = [this, choiceIndex]() {
        selectedHexTechChoice = choiceIndex;
        confirmHexTechButton->setEnabled(true);
        for (int i = 0; i < hexTechLayout->count(); ++i) {
            QWidget* widget = hexTechLayout->itemAt(i)->widget();
            if (!widget) {
                continue;
            }
            widget->setProperty("selected", widget->property("choiceIndex").toInt() == choiceIndex);
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
        }
    };
    return frame;
}

QWidget* EventPage::createRestOptionCard(RestOption option) {
    auto* frame = new RestCardFrame(this);
    frame->setFixedSize(300, 226);
    frame->setCursor(Qt::PointingHandCursor);
    frame->setAttribute(Qt::WA_StyledBackground, true);
    const bool trainDisabled = option == RestOption::Train && !hasTrainableCards();
    frame->setStyleSheet(R"(
        QFrame {
            background-color: rgba(255, 246, 182, 215);
            border: 3px solid rgba(255, 226, 115, 225);
            border-radius: 8px;
        }
        QFrame:hover {
            background-color: rgba(255, 252, 213, 240);
            border: 4px solid #ffb946;
        }
        QFrame:disabled {
            background-color: rgba(70, 64, 52, 150);
            border: 3px solid rgba(120, 105, 84, 150);
        }
    )");

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(0, 0, 0, 10);
    layout->setSpacing(8);

    auto* image = new QLabel(frame);
    image->setFixedSize(300, 170);
    image->setAlignment(Qt::AlignCenter);
    image->setStyleSheet("background: transparent; border: none;");
    const QPixmap pixmap = loadScaledPixmap(assetPath(restOptionImagePath(option)), image->size());
    if (!pixmap.isNull()) {
        image->setPixmap(pixmap);
    }
    layout->addWidget(image, 0, Qt::AlignCenter);

    auto* label = new QLabel(restOptionLabel(option), frame);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 25px; font-weight: 900; color: #5a310a; background: transparent; border: none;");
    layout->addWidget(label);

    if (trainDisabled) {
        auto* opacity = new QGraphicsOpacityEffect(frame);
        opacity->setOpacity(0.45);
        frame->setGraphicsEffect(opacity);
        frame->setCursor(Qt::ArrowCursor);
        frame->setToolTip(QString::fromUtf8("没有未到达三星的卡"));
    }

    frame->onHovered = [this, option, trainDisabled]() {
        restDescriptionLabel->setText(trainDisabled
                                          ? QString::fromUtf8("没有未到达三星的卡")
                                          : restOptionDescription(option));
    };
    frame->onLeft = [this]() {
        restDescriptionLabel->setText(QString::fromUtf8("选择一项行动"));
    };
    frame->onClicked = [this, option]() {
        emit restOptionSelected(option);
    };
    if (trainDisabled) {
        frame->onClicked = nullptr;
    }
    return frame;
}

QWidget* EventPage::createRestTrainingCard(int ownedCardIndex) {
    const OwnedCharacterCard& card = currentState.ownedCharacterCards[static_cast<size_t>(ownedCardIndex)];
    const auto info = characterfactory::infoFor(card.templateId);

    auto* frame = new RestCardFrame(this);
    frame->setFixedSize(220, 306);
    frame->setCursor(Qt::PointingHandCursor);
    frame->setAttribute(Qt::WA_StyledBackground, true);
    frame->setStyleSheet(R"(
        QFrame {
            background-color: rgba(255, 246, 182, 225);
            border: 3px solid rgba(255, 226, 115, 225);
            border-radius: 8px;
        }
        QFrame:hover {
            background-color: rgba(255, 252, 213, 245);
            border: 4px solid #ffb946;
        }
    )");

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(10, 10, 10, 12);
    layout->setSpacing(7);

    auto* image = new QLabel(frame);
    image->setFixedSize(196, 190);
    image->setAlignment(Qt::AlignCenter);
    image->setStyleSheet("background: #1b1813; border: 2px solid rgba(90, 49, 10, 180); border-radius: 6px;");
    if (info) {
        const QPixmap pixmap = loadScaledPixmap(characterCardPath(info->name), image->size());
        if (!pixmap.isNull()) {
            image->setPixmap(pixmap);
        }
    }
    layout->addWidget(image, 0, Qt::AlignCenter);

    auto* name = new QLabel(info ? QString::fromStdString(info->displayName) : QString::fromUtf8("未知角色"), frame);
    name->setAlignment(Qt::AlignCenter);
    name->setWordWrap(true);
    name->setStyleSheet("font-size: 19px; font-weight: 900; color: #4d2908; background: transparent; border: none;");
    layout->addWidget(name);

    auto* stars = new QLabel(QString::fromUtf8("%1 星 → %2 星").arg(card.starLevel).arg(card.starLevel + 1), frame);
    stars->setAlignment(Qt::AlignCenter);
    stars->setStyleSheet("font-size: 18px; font-weight: 900; color: #b36b00; background: transparent; border: none;");
    layout->addWidget(stars);

    frame->onHovered = [this, name]() {
        restDescriptionLabel->setText(QString::fromUtf8("让 %1 升一星").arg(name->text()));
    };
    frame->onLeft = [this]() {
        restDescriptionLabel->setText(QString::fromUtf8("选择一张未到达三星的卡，升一星"));
    };
    frame->onClicked = [this, ownedCardIndex]() {
        emit restTrainingCardSelected(ownedCardIndex);
    };
    return frame;
}

QWidget* EventPage::createEventOwnedCard(int ownedCardIndex) {
    const OwnedCharacterCard& card = currentState.ownedCharacterCards[static_cast<size_t>(ownedCardIndex)];
    const auto info = characterfactory::infoFor(card.templateId);

    auto* frame = new RestCardFrame(this);
    frame->setFixedSize(220, 306);
    frame->setCursor(Qt::PointingHandCursor);
    frame->setAttribute(Qt::WA_StyledBackground, true);
    frame->setStyleSheet(R"(
        QFrame {
            background-color: rgba(255, 246, 182, 225);
            border: 3px solid rgba(255, 226, 115, 225);
            border-radius: 8px;
        }
        QFrame:hover {
            background-color: rgba(255, 252, 213, 245);
            border: 4px solid #ffb946;
        }
    )");

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(10, 10, 10, 12);
    layout->setSpacing(7);

    auto* image = new QLabel(frame);
    image->setFixedSize(196, 190);
    image->setAlignment(Qt::AlignCenter);
    image->setStyleSheet("background: #1b1813; border: 2px solid rgba(90, 49, 10, 180); border-radius: 6px;");
    if (info) {
        const QPixmap pixmap = loadScaledPixmap(characterCardPath(info->name), image->size());
        if (!pixmap.isNull()) {
            image->setPixmap(pixmap);
        }
    }
    layout->addWidget(image, 0, Qt::AlignCenter);

    auto* name = new QLabel(info ? QString::fromStdString(info->displayName) : QString::fromUtf8("未知角色"), frame);
    name->setAlignment(Qt::AlignCenter);
    name->setWordWrap(true);
    name->setStyleSheet("font-size: 19px; font-weight: 900; color: #4d2908; background: transparent; border: none;");
    layout->addWidget(name);

    auto* stars = new QLabel(QString::fromUtf8("%1 星").arg(card.starLevel), frame);
    stars->setAlignment(Qt::AlignCenter);
    stars->setStyleSheet("font-size: 18px; font-weight: 900; color: #b36b00; background: transparent; border: none;");
    layout->addWidget(stars);

    frame->onHovered = [this, name]() {
        restDescriptionLabel->setText(QString::fromUtf8("选择 %1").arg(name->text()));
    };
    frame->onLeft = [this]() {
        restDescriptionLabel->setText(QString::fromStdString(currentState.currentEvent.selectionPrompt));
    };
    frame->onClicked = [this, ownedCardIndex]() {
        emit eventOwnedCardSelected(ownedCardIndex);
    };
    return frame;
}

bool EventPage::hasTrainableCards() const {
    for (const OwnedCharacterCard& card : currentState.ownedCharacterCards) {
        if (card.starLevel < 3) {
            return true;
        }
    }
    return false;
}

void EventPage::setSpecialEventLayoutVisible(bool visible) {
    if (!contentLayout) {
        return;
    }

    contentLayout->setContentsMargins(48, 12, 48, 32);
    titleLabel->setVisible(visible);
    bodyLabel->setVisible(visible);
    optionsWidget->setVisible(visible);
    restDescriptionLabel->setVisible(visible && restDescriptionLabel->isVisible());
    restScrollArea->setVisible(visible && restScrollArea->isVisible());
    titleLabel->setMaximumWidth(QWIDGETSIZE_MAX);
    bodyLabel->setMaximumWidth(QWIDGETSIZE_MAX);
    optionsWidget->setMaximumWidth(QWIDGETSIZE_MAX);
    restDescriptionLabel->setMaximumWidth(QWIDGETSIZE_MAX);
    restScrollArea->setMaximumWidth(QWIDGETSIZE_MAX);
    titleLabel->setMinimumWidth(0);
    bodyLabel->setMinimumWidth(0);
    optionsWidget->setMinimumWidth(0);
    restDescriptionLabel->setMinimumWidth(0);
    restScrollArea->setMinimumWidth(0);
    titleLabel->setAlignment(Qt::AlignCenter);
    bodyLabel->setAlignment(Qt::AlignCenter);
    optionsLayout->setAlignment(Qt::AlignCenter);
    contentLayout->setAlignment(titleLabel, Qt::AlignCenter);
    contentLayout->setAlignment(bodyLabel, Qt::AlignCenter);
    contentLayout->setAlignment(optionsWidget, Qt::AlignCenter);
    contentLayout->setAlignment(restDescriptionLabel, Qt::AlignCenter);
    contentLayout->setAlignment(restOptionsWidget, Qt::AlignCenter);
    contentLayout->setAlignment(restScrollArea, Qt::AlignCenter);
    titleLabel->setMinimumHeight(0);
    titleLabel->setMaximumHeight(QWIDGETSIZE_MAX);
    titleLabel->setStyleSheet("font-size: 32px; font-weight: 800; color: #ffffff; background: transparent;");
    bodyLabel->setMinimumHeight(0);
    bodyLabel->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    bodyLabel->setStyleSheet("font-size: 19px; color: #ffffff; background: transparent;");
    restDescriptionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    restDescriptionLabel->setFixedHeight(100);
    restDescriptionLabel->setStyleSheet(R"(
        font-size: 34px;
        font-weight: 900;
        color: #ff9d2e;
        background: transparent;
        padding: 12px 0;
    )");
}

void EventPage::updateBackground() {
    backgroundPixmap = {};
    if (!currentState.currentEvent.active || currentState.currentEvent.backgroundPath.empty()) {
        return;
    }

    const QString path = assetPath(currentState.currentEvent.backgroundPath);
    backgroundPixmap.load(path);
}
