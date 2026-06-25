#include "startpage.h"
#include "app/recordmanager.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDialog>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QStringList>
#include <QTextEdit>
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

static QString recordText(const std::vector<RunRecord>& records) {
    if (records.empty()) {
        return QString::fromUtf8("暂无历史战绩");
    }

    QString text;
    int index = 1;
    for (const RunRecord& record : records) {
        const QString result = record.result == "clear"
                                   ? QString::fromUtf8("通关")
                                   : QString::fromUtf8("死亡");
        QStringList names;
        for (const std::string& name : record.characterNames) {
            names << QString::fromStdString(name);
        }
        text += QString::fromUtf8("%1. %2\n层数：第 %3 层\n角色：%4\n用时：%5\n时间：%6\n\n")
                    .arg(index++)
                    .arg(result)
                    .arg(record.reachedLayer)
                    .arg(names.isEmpty() ? QString::fromUtf8("无") : names.join(QString::fromUtf8("、")))
                    .arg(formatElapsed(record.elapsedSeconds))
                    .arg(QString::fromStdString(record.finishedAt));
    }
    return text.trimmed();
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
    QToolButton* historyButton = makeImageButton(background, "history.png");
    QToolButton* settingsButton = makeImageButton(background, "setting.png");
    QToolButton* exitButton = makeImageButton(background, "exit.png");

    menuLayout->addWidget(newButton, 0, Qt::AlignHCenter);
    menuLayout->addWidget(continueButton, 0, Qt::AlignHCenter);
    menuLayout->addWidget(historyButton, 0, Qt::AlignHCenter);
    menuLayout->addWidget(settingsButton, 0, Qt::AlignHCenter);
    menuLayout->addWidget(exitButton, 0, Qt::AlignHCenter);
    overlay->addWidget(menuColumn, 0, Qt::AlignLeft);
    overlay->addStretch(40);

    root->addWidget(background);

    connect(newButton, &QToolButton::clicked, this, [this]() {
        emit newGameRequested(static_cast<int>(QRandomGenerator::global()->generate()));
    });
    connect(continueButton, &QToolButton::clicked, this, [this]() {
        emit loadGameRequested(1);
    });
    connect(historyButton, &QToolButton::clicked, this, [this]() {
        RecordManager manager;
        std::string error;
        const std::vector<RunRecord> records = manager.loadRecords(&error);
        if (!error.empty()) {
            QMessageBox::warning(this, QString::fromUtf8("历史战绩"), QString::fromStdString(error));
            return;
        }

        QDialog dialog(this);
        dialog.setWindowTitle(QString::fromUtf8("历史战绩"));
        dialog.resize(520, 460);
        auto* layout = new QVBoxLayout(&dialog);
        auto* text = new QTextEdit(&dialog);
        text->setReadOnly(true);
        text->setText(recordText(records));
        text->setStyleSheet("font-size: 16px; color: #241305; background: #f4dfad;");
        layout->addWidget(text);
        dialog.exec();
    });
    connect(settingsButton, &QToolButton::clicked, this, [this]() {
        QMessageBox::information(
            this,
            QString::fromUtf8("游戏说明"),
            QString::fromUtf8("在路线地图选择节点推进。\n"
                              "战斗前可购买角色、拖拽布阵、穿戴装备和合成装备。\n"
                              "战斗会自动进行，胜利后打开宝箱领取奖励。\n"
                              "顶部 HUD 可保存进度、查看背包、商店和装备。"));
    });
    connect(exitButton, &QToolButton::clicked, qApp, &QApplication::quit);
}
