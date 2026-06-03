#include "mappage.h"
#include "gui/widgets/gamehud.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>

static constexpr qreal mapSceneWidth = 1280.0;
static constexpr qreal mapSceneHeight = 2200.0;
static constexpr qreal mapContentRatio = 0.70;

MapPage::MapPage(QWidget* parent)
    : QWidget(parent)
    , hud(new GameHud(this))
    , view(new QGraphicsView(this))
    , scene(new QGraphicsScene(this)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(hud);
    layout->addWidget(view, 1);

    view->setScene(scene);
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    scene->setSceneRect(0, 0, mapSceneWidth, mapSceneHeight);

    connect(hud, &GameHud::saveRequested, this, &MapPage::saveRequested);
}

void MapPage::setState(const GameState& state) {
    hud->setState(state);
    const bool playEntryScroll = state.currentLayerId != lastAnimatedLayerId;
    if (playEntryScroll) {
        lastAnimatedLayerId = state.currentLayerId;
    }
    drawLayer(state, playEntryScroll);
}

QString MapPage::nodeIconPath(const MapNode& node) const {
    QString fileName;
    if (node.event_id == MapEventId::Start) {
        fileName = "node_start.png";
    } else if (node.event_id == MapEventId::Boss) {
        fileName = "node_boss.png";
    } else if (node.event_id == MapEventId::Rest) {
        fileName = "node_rest.png";
    } else if (node.event_id == MapEventId::NormalBattle) {
        fileName = "node_battle.png";
    } else if (node.event_id == MapEventId::EliteBattle) {
        fileName = "node_elite.png";
    } else {
        fileName = "node_event.png";
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath("assets/nodes/" + fileName);
}

void MapPage::drawLayer(const GameState& state, bool playEntryAnimation) {
    if (scrollAnimation) {
        scrollAnimation->stop();
        scrollAnimation->deleteLater();
        scrollAnimation = nullptr;
    }

    scene->clear();
    const MapLayer* layer = state.map.layerById(state.currentLayerId);
    if (!layer) {
        return;
    }

    const QRectF bounds(0, 0, mapSceneWidth, mapSceneHeight);
    const qreal contentWidth = mapSceneWidth * mapContentRatio;
    const qreal contentLeft = (mapSceneWidth - contentWidth) * 0.5;
    const QRectF mapBounds(contentLeft, 0, contentWidth, mapSceneHeight);
    scene->setSceneRect(bounds);
    const QString imagePath = QDir(QCoreApplication::applicationDirPath()).filePath(QString::fromStdString(layer->backgroundPath));
    QPixmap background(imagePath);
    if (!background.isNull()) {
        QPixmap scaled = background.scaled(mapBounds.size().toSize(), Qt::KeepAspectRatioByExpanding,
                                           Qt::SmoothTransformation);
        QGraphicsPixmapItem* bgItem = scene->addPixmap(scaled);
        bgItem->setPos(mapBounds.left() + (mapBounds.width() - scaled.width()) * 0.5, 0);
        bgItem->setZValue(-10);
        bgItem->setOpacity(0.7);
    } else {
        auto* rect = scene->addRect(mapBounds, QPen(Qt::NoPen), QColor(36, 39, 45));
        rect->setZValue(-10);
        auto* title = scene->addText(QString("Layer %1 background placeholder").arg(layer->layerId));
        title->setDefaultTextColor(QColor(210, 210, 210));
        title->setPos(24, 20);
        title->setZValue(-9);
    }

    auto* sideFill = scene->addRect(bounds, QPen(Qt::NoPen), QColor(16, 17, 22));
    sideFill->setZValue(-12);
    sideFill->setRect(bounds);

    auto* shade = scene->addRect(bounds, QPen(Qt::NoPen), QColor(0, 0, 0, 55));
    shade->setZValue(-8);

    for (const MapNode& node : layer->nodes) {
        for (int nextId : node.nextNodeIds) {
            const MapNode* next = layer->nodeById(nextId);
            if (!next) {
                continue;
            }
            const QPointF fromPoint(node.position.first, node.position.second);
            const QPointF toPoint(next->position.first, next->position.second);
            auto* line = scene->addLine(QLineF(fromPoint, toPoint),
                                        QPen(QColor(165, 165, 165, 180), 4, Qt::DashLine,
                                             Qt::RoundCap, Qt::RoundJoin));
            line->setZValue(0);
        }
    }

    for (const MapNode& node : layer->nodes) {
        const bool start = node.event_id == MapEventId::Start;
        const bool end = node.event_id == MapEventId::Boss;
        const bool elite = node.event_id == MapEventId::EliteBattle;
        const int size = start ? 96 : (end ? 76 : (elite ? 64 : 56));
        const bool available = std::find(state.availableNodeIds.begin(), state.availableNodeIds.end(), node.id) != state.availableNodeIds.end();
        const bool completed = std::find(state.completedNodeIds.begin(), state.completedNodeIds.end(), node.id) != state.completedNodeIds.end();
        const bool current = node.id == state.playerNodeId;
        const bool fullOpacity = available || (current && !completed);
        const QPointF nodePoint(node.position.first, node.position.second);

        QPixmap icon(nodeIconPath(node));
        if (!icon.isNull()) {
            auto* iconItem = scene->addPixmap(icon.scaled(size, size, Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation));
            iconItem->setPos(nodePoint - QPointF(size * 0.5, size * 0.5));
            iconItem->setOpacity(fullOpacity ? 1.0 : 0.5);
            iconItem->setZValue(10);
        }

        auto* button = new QToolButton();
        button->setEnabled(available);
        button->setFixedSize(size, size);
        button->setCursor(available ? Qt::PointingHandCursor : Qt::ArrowCursor);
        button->setAutoRaise(true);
        button->setStyleSheet(
            "QToolButton { background: transparent; border: 0; }"
            "QToolButton:hover:enabled { background-color: rgba(255,255,255,20); }"
            "QToolButton:disabled { background: transparent; border: 0; }");
        connect(button, &QToolButton::clicked, this, [this, node]() {
            emit nodeSelected(node.id);
        });

        QGraphicsProxyWidget* proxy = scene->addWidget(button);
        proxy->setPos(nodePoint - QPointF(size * 0.5, size * 0.5));
        proxy->setZValue(10);

        if (node.id == state.playerNodeId) {
            const int ringSize = size + 32;
            QPixmap ring(QDir(QCoreApplication::applicationDirPath()).filePath("assets/nodes/node_current_ring.png"));
            if (!ring.isNull()) {
                auto* ringItem = scene->addPixmap(ring.scaled(ringSize, ringSize,
                                                              Qt::KeepAspectRatio,
                                                              Qt::SmoothTransformation));
                ringItem->setPos(nodePoint - QPointF(ringSize * 0.5, ringSize * 0.5));
                ringItem->setZValue(13);
            } else {
                auto* ringItem = scene->addEllipse(QRectF(node.position.first - ringSize * 0.5,
                                                          node.position.second - ringSize * 0.5,
                                                          ringSize, ringSize),
                                                   QPen(QColor(220, 35, 35), 6), Qt::NoBrush);
                ringItem->setZValue(13);
            }
        }
    }

    if (playEntryAnimation) {
        QTimer::singleShot(0, this, &MapPage::playEntryScroll);
        return;
    }

    const MapNode* playerNode = layer->nodeById(state.playerNodeId);
    if (playerNode) {
        view->centerOn(QPointF(playerNode->position.first, playerNode->position.second));
    }
}

void MapPage::playEntryScroll() {
    QScrollBar* scrollBar = view->verticalScrollBar();
    if (!scrollBar) {
        return;
    }

    scrollBar->setValue(scrollBar->minimum());

    scrollAnimation = new QPropertyAnimation(scrollBar, "value", this);
    scrollAnimation->setDuration(1800);
    scrollAnimation->setStartValue(scrollBar->minimum());
    scrollAnimation->setEndValue(scrollBar->maximum());
    scrollAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    connect(scrollAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (scrollAnimation) {
            scrollAnimation->deleteLater();
            scrollAnimation = nullptr;
        }
    });
    scrollAnimation->start();
}
