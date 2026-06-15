#include "eventpage.h"
#include "gui/widgets/gamehud.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPushButton>
#include <QStyle>
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

static QString assetPath(const std::string& relativePath) {
    const QString path = QString::fromStdString(relativePath);
    const QString runtimePath = QDir(QCoreApplication::applicationDirPath()).filePath(path);
    if (QFile::exists(runtimePath)) {
        return runtimePath;
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(path);
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
    , titleLabel(new QLabel(this))
    , bodyLabel(new QLabel(this))
    , optionsLayout(nullptr)
    , hexTechLayout(nullptr)
    , confirmHexTechButton(new QPushButton(QString::fromUtf8("确认选择"), this))
    , selectedHexTechChoice(-1) {
    setAutoFillBackground(false);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(hud);

    auto* content = new QWidget(this);
    content->setAttribute(Qt::WA_TranslucentBackground);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(48, 24, 48, 32);
    contentLayout->setSpacing(12);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 32px; font-weight: 800; color: #ffffff; background: transparent;");
    contentLayout->addWidget(titleLabel);

    bodyLabel->setAlignment(Qt::AlignCenter);
    bodyLabel->setWordWrap(true);
    bodyLabel->setStyleSheet("font-size: 19px; color: #ffffff; background: transparent;");
    contentLayout->addWidget(bodyLabel);

    auto* optionsWidget = new QWidget(this);
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
    hexTechLayout->setSpacing(42);
    hexTechOuterLayout->addLayout(hexTechLayout);
    confirmHexTechButton->setEnabled(false);
    confirmHexTechButton->setVisible(false);
    confirmHexTechButton->setMinimumWidth(180);
    hexTechOuterLayout->addWidget(confirmHexTechButton, 0, Qt::AlignCenter);
    contentLayout->addWidget(hexTechWidget);

    contentLayout->addStretch();
    layout->addWidget(content, 1);

    connect(hud, &GameHud::saveRequested, this, &EventPage::saveRequested);
    connect(hud, &GameHud::bagRequested, this, &EventPage::bagRequested);
    connect(hud, &GameHud::shopRequested, this, &EventPage::shopRequested);
    connect(confirmHexTechButton, &QPushButton::clicked, this, [this]() {
        if (selectedHexTechChoice >= 0) {
            emit hexTechCardSelected(selectedHexTechChoice);
        }
    });

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
    selectedHexTechChoice = -1;
    confirmHexTechButton->setEnabled(false);
    confirmHexTechButton->setVisible(false);

    if (!currentState.currentEvent.active) {
        return;
    }

    if (currentState.currentEvent.hexTechSelection) {
        confirmHexTechButton->setVisible(true);
        for (int i = 0; i < static_cast<int>(currentState.currentHexTechChoices.size()); ++i) {
            hexTechLayout->addWidget(createHexTechCard(i));
        }
        return;
    }

    for (int i = 0; i < static_cast<int>(currentState.currentEvent.options.size()); ++i) {
        const EventOption& option = currentState.currentEvent.options[static_cast<size_t>(i)];
        auto* button = new QPushButton(QString::fromStdString(option.label), this);
        button->setMinimumWidth(240);
        optionsLayout->addWidget(button, 0, Qt::AlignCenter);
        connect(button, &QPushButton::clicked, this, [this, i]() {
            emit eventOptionSelected(i);
        });
    }
}

QWidget* EventPage::createHexTechCard(int choiceIndex) {
    const HexTechDefinition& choice = currentState.currentHexTechChoices[static_cast<size_t>(choiceIndex)];

    auto* frame = new HexTechCardFrame(this);
    frame->setFixedWidth(270);
    frame->setCursor(Qt::PointingHandCursor);
    frame->setProperty("choiceIndex", choiceIndex);
    frame->setStyleSheet(R"(
        QFrame {
            background-color: rgba(10, 12, 16, 125);
            border: 2px solid rgba(230, 210, 150, 95);
            border-radius: 8px;
        }
        QFrame[selected="true"] {
            border: 3px solid #f0c85a;
            background-color: rgba(36, 29, 16, 150);
        }
    )");

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(12, 12, 12, 14);
    layout->setSpacing(10);

    auto* imageLabel = new QLabel(frame);
    imageLabel->setFixedSize(230, 322);
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
    title->setStyleSheet("font-size: 22px; font-weight: 800; color: #ffffff; border: none; background: transparent;");
    layout->addWidget(title);

    auto* description = new QLabel(QString::fromStdString(choice.description), frame);
    description->setWordWrap(true);
    description->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    description->setMinimumHeight(78);
    description->setStyleSheet("font-size: 17px; color: #ffffff; border: none; background: transparent;");
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

void EventPage::updateBackground() {
    backgroundPixmap = {};
    if (!currentState.currentEvent.active || currentState.currentEvent.backgroundPath.empty()) {
        return;
    }

    const QString path = assetPath(currentState.currentEvent.backgroundPath);
    backgroundPixmap.load(path);
}
