#include "battlepage.h"
#include "combat/battlescene.h"
#include "entity/character/characterfactory.h"
#include "gui/widgets/gamehud.h"
#include "world/hextech/hextechrepository.h"
#include <QEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QIcon>
#include <QPainter>
#include <QPushButton>
#include <QSize>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

BattlePage::BattlePage(QWidget* parent)
    : QWidget(parent)
    , hud(new GameHud(this))
    , view(new QGraphicsView(this))
    , startButton(new QPushButton(this))
    , settlementButton(new QPushButton(view))
    , game(new BattleScene(this))
    , battleSystem(nullptr)
    , battleTimer(new QTimer(this))
    , pendingBattleResult({})
    , settlementActive(false)
    , chestSettlementPending(false) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(hud);
    layout->addWidget(view, 1);

    auto* controls = new QWidget(this);
    auto* controlsLayout = new QHBoxLayout(controls);
    controlsLayout->setContentsMargins(8, 6, 8, 6);
    controlsLayout->addStretch();
    controlsLayout->addWidget(startButton);
    layout->addWidget(controls);

    startButton->setIcon(QIcon("assets/ui/start_battle.png"));
    startButton->setIconSize(QSize(210, 90));
    startButton->setFixedSize(230, 96);
    startButton->setFlat(true);
    startButton->setToolTip("Start battle");

    settlementButton->setIcon(QIcon("assets/ui/next_settlement.png"));
    settlementButton->setIconSize(QSize(280, 120));
    settlementButton->setFixedSize(300, 126);
    settlementButton->setFlat(true);
    settlementButton->setToolTip("Next stage");
    settlementButton->hide();
    settlementButton->raise();
    view->installEventFilter(this);

    game->initialize();
    ensureStarterRoster();
    view->setScene(game->scene());
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    battleTimer->setInterval(600);
    connect(battleTimer, &QTimer::timeout, this, [this]() {
        if (battleSystem && battleSystem->isRunning()) {
            battleSystem->update(0.6f);
        }
    });

    connect(startButton, &QPushButton::clicked, this, [this]() {
        emit panelsShouldClose();
        emit startBattleRequested();
    });
    connect(settlementButton, &QPushButton::clicked, this, &BattlePage::completeSettlement);
    connect(game, &BattleScene::rewardChestOpened, this, &BattlePage::completeVictoryChest);
    connect(hud, &GameHud::saveRequested, this, &BattlePage::saveRequested);
    connect(hud, &GameHud::bagRequested, this, &BattlePage::bagRequested);
    connect(hud, &GameHud::shopRequested, this, &BattlePage::shopRequested);
    connect(hud, &GameHud::returnToStartRequested, this, &BattlePage::returnToStartRequested);

    QTimer::singleShot(0, this, &BattlePage::fitSceneInView);
}

BattlePage::~BattlePage() = default;

void BattlePage::setBattleSystem(battlesystem* system) {
    if (battleSystem == system) {
        return;
    }

    if (battleSystem) {
        disconnect(battleSystem, nullptr, this, nullptr);
        disconnect(battleSystem, nullptr, game, nullptr);
    }

    battleSystem = system;
    if (!battleSystem) {
        return;
    }

    game->setBoard(&battleSystem->mutableBoard());

    connect(battleSystem, &battlesystem::boardChanged,
            this, &BattlePage::syncFromBattleSystem);
    connect(battleSystem, &battlesystem::characterRemoved,
            game, &BattleScene::removeCharacterItem);
    connect(battleSystem, &battlesystem::battleStarted, this, [this]() {
        startButton->hide();
        syncFromBattleSystem();
        battleTimer->start();
    });
    connect(battleSystem, &battlesystem::battleEnded, this, [this](bool pcTeamAllDead) {
        battleTimer->stop();
        beginSettlement(!pcTeamAllDead);
    });
}

std::vector<CharacterPlacement> BattlePage::preparedPlacements() const {
    return game->preparedPlacements();
}

void BattlePage::setState(const GameState& state) {
    currentState = state;
    hud->setState(state);
    if (game) {
        game->setDeploymentLimit(deploymentLimitForCurrentLayer());
    }
    syncRosterFromState();
}

void BattlePage::setElapsedSeconds(int seconds) {
    hud->setElapsedSeconds(seconds);
}

void BattlePage::startNodeBattle(int) {
    settlementActive = false;
    chestSettlementPending = false;
    settlementButton->hide();
    if (battleSystem) {
        game->setBoard(&battleSystem->mutableBoard());
        game->setDeploymentLimit(deploymentLimitForCurrentLayer());
        syncRosterFromState();
        syncFromBattleSystem();
    }
    startButton->show();
    QTimer::singleShot(0, this, &BattlePage::fitSceneInView);
}

void BattlePage::fitSceneInView() {
    if (view && view->scene()) {
        view->fitInView(view->scene()->sceneRect(), Qt::KeepAspectRatio);
    }
    positionSettlementButton();
}

void BattlePage::ensureStarterRoster() {
    if (!roster.empty()) {
        return;
    }

    for (int templateId : {1209}) {
        std::unique_ptr<Character> character = characterfactory::create(templateId);
        if (!character) {
            continue;
        }
        Character* raw = character.get();
        roster.push_back(std::move(character));
        game->addCharacter(raw, raw->name());
    }
}

void BattlePage::syncRosterFromState() {
    if (!game || currentState.ownedCharacterCards.empty()) {
        ensureStarterRoster();
        return;
    }
    if (battleSystem && battleSystem->isRunning()) {
        return;
    }
    if (rosterSignature.size() == currentState.ownedCharacterCards.size() &&
        std::equal(rosterSignature.begin(), rosterSignature.end(), currentState.ownedCharacterCards.begin(),
                   [](const OwnedCharacterCard& left, const OwnedCharacterCard& right) {
                       return left.templateId == right.templateId && left.starLevel == right.starLevel;
                   })) {
        return;
    }

    roster.clear();
    rosterSignature = currentState.ownedCharacterCards;
    game->clearRoster();
    for (const OwnedCharacterCard& card : currentState.ownedCharacterCards) {
        std::unique_ptr<Character> character = characterfactory::create(card.templateId);
        if (!character) {
            continue;
        }
        applyStarScaling(character.get(), card.starLevel);
        Character* raw = character.get();
        roster.push_back(std::move(character));
        game->addCharacter(raw, raw->name());
    }
}

void BattlePage::applyStarScaling(Character* character, int starLevel) {
    if (!character || starLevel <= 1) {
        return;
    }
    const double multiplier = 1.0 + 0.2 * (std::min(3, starLevel) - 1);
    StatsComponent& stats = character->getstats();
    const int maxHp = std::max(1, static_cast<int>(std::lround(stats.getMAXHP() * multiplier)));
    const int attack = std::max(0, static_cast<int>(std::lround(stats.getATTACK() * multiplier)));
    const int defense = std::max(0, static_cast<int>(std::lround(stats.getDEFENSE() * multiplier)));
    stats.modifyMAXHP(maxHp - stats.getMAXHP());
    stats.setHP(maxHp);
    stats.setATTACK(attack);
    stats.modifyDEFESE(defense - stats.getDEFENSE());
}

int BattlePage::deploymentLimitForCurrentLayer() const {
    if (currentState.currentLayerId <= 1) {
        return 4;
    }
    if (currentState.currentLayerId == 2) {
        return 5;
    }
    return 6;
}

void BattlePage::syncFromBattleSystem() {
    if (battleSystem && !settlementActive) {
        game->syncFromBattleSystem(*battleSystem);
    }
}

bool BattlePage::eventFilter(QObject* watched, QEvent* event) {
    if (watched == view && event->type() == QEvent::Resize) {
        positionSettlementButton();
    }
    return QWidget::eventFilter(watched, event);
}

void BattlePage::positionSettlementButton() {
    if (!settlementButton || !view) {
        return;
    }

    const int x = (view->viewport()->width() - settlementButton->width()) / 2;
    const int y = view->viewport()->height() - settlementButton->height() - 44;
    settlementButton->move(qMax(0, x), qMax(0, y));
}

void BattlePage::beginSettlement(bool victory) {
    settlementActive = true;
    pendingBattleResult = {victory};
    startButton->hide();
    chestSettlementPending = false;
    if (victory) {
        settlementButton->hide();
        const int seedSalt = currentState.seed * 1103515245 +
                             currentState.currentLayerId * 4099 +
                             currentState.currentNodeId * 9176;
        chestSettlementPending = game && game->spawnRewardChest(seedSalt);
        if (chestSettlementPending) {
            return;
        }
    }
    positionSettlementButton();
    settlementButton->show();
    settlementButton->raise();
}

void BattlePage::completeSettlement() {
    settlementButton->hide();
    settlementActive = false;
    chestSettlementPending = false;
    emit battleFinished(pendingBattleResult);
}

void BattlePage::completeVictoryChest() {
    if (!chestSettlementPending) {
        return;
    }
    chestSettlementPending = false;
    emit battleChestOpened(victoryChestGold());
    completeSettlement();
}

int BattlePage::victoryChestGold() const {
    int gold = 2 + currentState.currentLayerId;
    if (currentState.currentBattle.kind == BattleKind::Elite) {
        gold *= 2;
    }
    gold += extraChestGoldFromHexTech();
    return gold;
}

int BattlePage::extraChestGoldFromHexTech() const {
    int extra = 0;
    HexTechRepository repository;
    for (const HexTechCardRecord& record : currentState.selectedHexTechCards) {
        const auto definition = repository.findById(record.hexTechId);
        if (!definition) {
            continue;
        }
        for (const HexTechEffect& effect : definition->effects) {
            if (effect.type == "extra_chest_gold") {
                extra += effect.value;
            }
        }
    }
    return extra;
}


