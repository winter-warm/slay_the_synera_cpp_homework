#include "gamewindow.h"
#include "core/game.h"
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

static constexpr int windowWidth = 1280, windowHeight = 720;

GameWindow::GameWindow(QWidget* parent)
    : QMainWindow(parent)
    , centralWidget(new QWidget(this))
    , mainLayout(new QVBoxLayout())
    , view(new QGraphicsView(this))
    , resetButton(new QPushButton("Reset", this))
    , game(new Game(this)) {
    setupUI();
    game->initialize();
    QTimer::singleShot(0, this, &GameWindow::fitSceneInView);
}

GameWindow::~GameWindow() = default;

void GameWindow::onResetButtonClicked() {
    if (game) {
        game->reset();
    }
}

void GameWindow::setupUI() {
    setFixedSize(windowWidth, windowHeight);
    setCentralWidget(centralWidget);
    centralWidget->setLayout(mainLayout);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    setStyleSheet(R"(
        QMainWindow {
            background-color: #2b2b2b;
        }
        QWidget {
            background-color: #2b2b2b;
            color: #f2f2f2;
        }
        QPushButton {
            background-color: #2f2f2f;
            color: #f2f2f2;
            border: 1px solid #565656;
            border-radius: 4px;
            padding: 6px 14px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #3a3a3a;
        }
        QPushButton:pressed {
            background-color: #242424;
        }
    )");

    view->setRenderHint(QPainter::Antialiasing, true);
    view->setDragMode(QGraphicsView::NoDrag);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    view->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    view->setAlignment(Qt::AlignCenter);
    view->setMouseTracking(true);
    view->viewport()->setMouseTracking(true);

    mainLayout->addWidget(view, 1);

    QWidget* controlBar = new QWidget(this);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->addWidget(resetButton);
    controlLayout->addStretch();
    mainLayout->addWidget(controlBar);

    connect(resetButton, &QPushButton::clicked,
            this, &GameWindow::onResetButtonClicked);

    view->setScene(game->scene());
}

void GameWindow::fitSceneInView() {
    if (!view || !view->scene()) {
        return;
    }

    const QRectF sceneRect = view->scene()->sceneRect();
    if (sceneRect.isEmpty()) {
        return;
    }

    view->resetTransform();
    view->fitInView(sceneRect, Qt::KeepAspectRatio);
}

void GameWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    fitSceneInView();
}
