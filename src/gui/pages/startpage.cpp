#include "startpage.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

class StartBackground : public QWidget {
public:
    explicit StartBackground(QWidget* parent = nullptr)
        : QWidget(parent)
        , background(QDir(QCoreApplication::applicationDirPath()).filePath("assets/pages/starter.png")) {}

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        if (background.isNull()) {
            painter.fillRect(rect(), QColor(28, 29, 34));
            return;
        }

        const QSize target = size();
        QPixmap scaled = background.scaled(target, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const QPoint topLeft((width() - scaled.width()) / 2, (height() - scaled.height()) / 2);
        painter.drawPixmap(topLeft, scaled);
    }

private:
    QPixmap background;
};

static QToolButton* makeImageButton(QWidget* parent, const QString& assetName) {
    auto* button = new QToolButton(parent);
    const QString path = QDir(QCoreApplication::applicationDirPath()).filePath("assets/ui/" + assetName);
    button->setIcon(QIcon(path));
    button->setIconSize(QSize(220, 82));
    button->setFixedSize(236, 90);
    button->setAutoRaise(true);
    button->setCursor(Qt::PointingHandCursor);
    auto* opacity = new QGraphicsOpacityEffect(button);
    opacity->setOpacity(0.9);
    button->setGraphicsEffect(opacity);
    button->setStyleSheet(R"(
        QToolButton {
            background: transparent;
            border: none;
        }
        QToolButton:hover {
            background-color: rgba(255, 255, 255, 24);
            border-radius: 10px;
        }
        QToolButton:pressed {
            background-color: rgba(0, 0, 0, 35);
            border-radius: 10px;
        }
    )");
    return button;
}

StartPage::StartPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto* background = new StartBackground(this);
    auto* overlay = new QVBoxLayout(background);
    overlay->setContentsMargins(120, 0, 0, 0);
    overlay->setSpacing(10);

    overlay->addStretch(38);

    auto* menuColumn = new QWidget(background);
    menuColumn->setFixedWidth(690);
    auto* menuLayout = new QVBoxLayout(menuColumn);
    menuLayout->setContentsMargins(0, 0, 0, 0);
    menuLayout->setSpacing(10);

    auto* title = new QLabel(background);
    title->setFixedSize(690, 300);
    title->setScaledContents(true);
    title->setPixmap(QPixmap(QDir(QCoreApplication::applicationDirPath()).filePath("assets/ui/title.png")));
    auto* titleOpacity = new QGraphicsOpacityEffect(title);
    titleOpacity->setOpacity(0.8);
    title->setGraphicsEffect(titleOpacity);
    menuLayout->addWidget(title, 0, Qt::AlignHCenter);
    menuLayout->addSpacing(48);

    QToolButton* newButton = makeImageButton(background, "newgame.png");
    QToolButton* continueButton = makeImageButton(background, "continuegame.png");
    QToolButton* settingsButton = makeImageButton(background, "setting.png");
    QToolButton* exitButton = makeImageButton(background, "exit.png");

    menuLayout->addWidget(newButton, 0, Qt::AlignHCenter);
    menuLayout->addWidget(continueButton, 0, Qt::AlignHCenter);
    menuLayout->addWidget(settingsButton, 0, Qt::AlignHCenter);
    menuLayout->addWidget(exitButton, 0, Qt::AlignHCenter);
    overlay->addWidget(menuColumn, 0, Qt::AlignLeft);
    overlay->addStretch(40);

    root->addWidget(background);

    connect(newButton, &QToolButton::clicked, this, [this]() {
        emit newGameRequested(static_cast<int>(QRandomGenerator::global()->generate()));
    });
    connect(continueButton, &QToolButton::clicked, this, [this]() {
        bool ok = false;
        const int slot = QInputDialog::getInt(this, "Continue Game", "Save slot:", 1, 1, 3, 1, &ok);
        if (ok) {
            emit loadGameRequested(slot);
        }
    });
    connect(settingsButton, &QToolButton::clicked, this, [this]() {
        QMessageBox::information(this, "Settings", "Settings placeholder.");
    });
    connect(exitButton, &QToolButton::clicked, qApp, &QApplication::quit);
}
