#include "battlepage.h"
#include "combat/battlescene.h"
#include "entity/character/characterfactory.h"
#include "gui/widgets/gamehud.h"
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

BattlePage::BattlePage(QWidget* parent)
    : QWidget(parent)
    , hud(new GameHud(this))
    , view(new QGraphicsView(this))
    , startButton(new QPushButton("Start Battle", this))
    , winButton(new QPushButton("Finish Battle", this))
    , game(new BattleScene(this)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(hud);
    layout->addWidget(view, 1);

    auto* controls = new QWidget(this);
    auto* controlsLayout = new QHBoxLayout(controls);
    controlsLayout->setContentsMargins(8, 6, 8, 6);
    controlsLayout->addWidget(startButton);
    controlsLayout->addWidget(winButton);
    controlsLayout->addStretch();
    layout->addWidget(controls);

    game->initialize();
    ensureStarterRoster();
    view->setScene(game->scene());
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(startButton, &QPushButton::clicked, game, &BattleScene::startBattle);
    connect(winButton, &QPushButton::clicked, this, [this]() {
        game->endBattle();
        emit battleFinished({});
    });
    connect(hud, &GameHud::saveRequested, this, &BattlePage::saveRequested);

    QTimer::singleShot(0, this, &BattlePage::fitSceneInView);
}

BattlePage::~BattlePage() = default;

void BattlePage::setState(const GameState& state) {
    hud->setState(state);
}

void BattlePage::startNodeBattle(int) {
    game->reset();
    QTimer::singleShot(0, this, &BattlePage::fitSceneInView);
}

void BattlePage::fitSceneInView() {
    if (view && view->scene()) {
        view->fitInView(view->scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}

void BattlePage::ensureStarterRoster() {
    if (!roster.empty()) {
        return;
    }

    for (int templateId : {1001}) {
        std::unique_ptr<Character> character = characterfactory::create(templateId);
        if (!character) {
            continue;
        }
        Character* raw = character.get();
        roster.push_back(std::move(character));
        game->addCharacter(raw, raw->name());
    }
}


