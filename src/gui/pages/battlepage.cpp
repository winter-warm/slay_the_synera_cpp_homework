#include "battlepage.h"
#include "gui/battle/battlescene.h"
#include "combat/equipment/equipmentrepository.h"
#include "entity/character/character.h"
#include "entity/character/characterfactory.h"
#include "entity/unit.h"
#include "gui/widgets/gamehud.h"
#include "world/hextech/hextechrepository.h"
#include <QEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QGraphicsOpacityEffect>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMimeData>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSize>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <random>

static const char* equipmentMimeType = "application/x-synera-equipment-instance";
static const char* reachedBondColor = "#8a6500";
static const char* unreachedBondColor = "#8f8f8f";
static const char* normalBondColor = "#16120f";

static QString bondDisplayName(Bond bond)
{
    switch (bond) {
    case Bond::soldier:
        return QString::fromUtf8("战士");
    case Bond::mage:
        return QString::fromUtf8("法师");
    case Bond::animal:
        return QString::fromUtf8("动物");
    case Bond::building:
        return QString::fromUtf8("建筑");
    case Bond::pastor:
        return QString::fromUtf8("牧师");
    case Bond::witch:
        return QString::fromUtf8("魔女");
    case Bond::beast:
        return QString::fromUtf8("野兽");
    case Bond::chess:
        return QString::fromUtf8("棋子");
    case Bond::assassin:
        return QString::fromUtf8("刺客");
    case Bond::none:
        return QString();
    }
    return QString();
}

struct BondEffectText {
    int threshold = 0;
    const char* description = "";
};

static std::vector<BondEffectText> bondEffectTexts(Bond bond)
{
    switch (bond) {
    case Bond::soldier:
        return {{3, "全体攻击+10%"}, {5, "全体攻击+20%，生命+10%"}};
    case Bond::witch:
        return {{3, "全体生命-30%，攻击+50%"}, {5, "全体防御+10%，攻击+30%"}};
    case Bond::assassin:
        return {{3, "全体攻速+50%"}, {5, "刺客生命+20%"}};
    case Bond::mage:
        return {{2, "全体初始MP+3"}, {4, "全体最大MP-1"}};
    case Bond::building:
        return {{1, "全体生命+30%"}};
    default:
        return {};
    }
}

static QString coloredBondNumber(int value, bool reached)
{
    return QString("<span style=\"color:%1; font-weight:900; min-width:18px;\">%2</span>")
        .arg(reached ? reachedBondColor : unreachedBondColor)
        .arg(value);
}

class ZoneLegendWidget : public QWidget {
public:
    explicit ZoneLegendWidget(QWidget* parent = nullptr)
        : QWidget(parent) {
        setFixedSize(190, 184);
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        painter.setPen(QPen(QColor(32, 28, 24, 210), 2));
        painter.setBrush(QColor(248, 241, 225, 232));
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);

        QFont titleFont = painter.font();
        titleFont.setPointSize(13);
        titleFont.setBold(true);
        painter.setFont(titleFont);
        painter.setPen(QColor(36, 30, 24));
        painter.drawText(QRectF(18, 10, width() - 36, 24), Qt::AlignLeft | Qt::AlignVCenter,
                         QString::fromUtf8("格子说明"));

        struct Entry {
            QColor color;
            const char* label;
        };
        const Entry entries[] = {
            {QColor(60, 60, 80), "我方布阵"},
            {QColor(80, 60, 60), "敌方区域"},
            {QColor(180, 160, 105), "战斗区域"},
            {QColor(70, 125, 75), "阻挡障碍"},
            {QColor(130, 105, 40), "低矮障碍"}
        };

        QFont labelFont = painter.font();
        labelFont.setPointSize(11);
        labelFont.setBold(true);
        painter.setFont(labelFont);

        for (int row = 0; row < 5; ++row) {
            const qreal centerX = 28.0;
            const qreal centerY = 54.0 + row * 25.0;
            QPolygonF hex;
            hex.reserve(6);
            for (int i = 0; i < 6; ++i) {
                const qreal angle = (60.0 * i - 90.0) * 3.14159265358979323846 / 180.0;
                hex.append(QPointF(centerX + std::cos(angle) * 11.0,
                                   centerY + std::sin(angle) * 11.0));
            }

            painter.setPen(QPen(QColor(36, 36, 36), 1.5));
            painter.setBrush(entries[row].color);
            painter.drawPolygon(hex);

            painter.setPen(QColor(36, 30, 24));
            painter.drawText(QRectF(50, centerY - 11.0, 120, 22),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             QString::fromUtf8(entries[row].label));
        }
    }
};

BattlePage::BattlePage(QWidget* parent)
    : QWidget(parent)
    , hud(new GameHud(this))
    , view(new QGraphicsView(this))
    , startButton(new QPushButton(this))
    , settlementButton(new QPushButton(view))
    , equipmentWarningLabel(new QLabel(view))
    , zoneLegend(new ZoneLegendWidget(view))
    , bondSummaryLabel(nullptr)
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
    equipmentWarningLabel->setText(QString::fromUtf8("ta已经穿了装备了"));
    equipmentWarningLabel->setAlignment(Qt::AlignCenter);
    equipmentWarningLabel->setStyleSheet("font-size: 40px; font-weight: 900; color: #ffcc38; background: rgba(28, 18, 8, 180); border-radius: 12px;");
    equipmentWarningLabel->setFixedSize(430, 86);
    equipmentWarningLabel->hide();
    equipmentWarningLabel->raise();
    zoneLegend->show();
    zoneLegend->raise();
    view->installEventFilter(this);
    view->viewport()->installEventFilter(this);
    view->viewport()->setAcceptDrops(true);

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
    connect(game, &BattleScene::placementsChanged, this, &BattlePage::updateBondSummary);
    connect(game, &BattleScene::rosterUnitRecycleRequested, this, [this](int unitId) {
        const auto uidIt = unitIdToOwnedCardUid.find(unitId);
        if (uidIt != unitIdToOwnedCardUid.end()) {
            emit recycleOwnedCardRequested(uidIt->second);
        }
    });
    connect(game, &BattleScene::rewardChestOpened, this, &BattlePage::completeVictoryChest);
    connect(game, &BattleScene::rewardEquipmentClaimed, this, [this](EquipmentGroup group, int equipmentId) {
        auto it = std::find_if(pendingRewardEquipmentDrops.begin(), pendingRewardEquipmentDrops.end(),
                               [group, equipmentId](const BattleScene::EquipmentRewardVisual& drop) {
                                   return drop.group == group && drop.equipmentId == equipmentId;
                               });
        if (it != pendingRewardEquipmentDrops.end()) {
            pendingRewardEquipmentDrops.erase(it);
        }
        emit battleChestEquipmentClaimed(group, equipmentId);
        pendingRewardEquipmentClaims = static_cast<int>(pendingRewardEquipmentDrops.size());
        if (chestSettlementPending && pendingRewardEquipmentClaims <= 0) {
            completeSettlement();
        }
    });
    connect(hud, &GameHud::saveRequested, this, &BattlePage::saveRequested);
    connect(hud, &GameHud::bagRequested, this, &BattlePage::bagRequested);
    connect(hud, &GameHud::shopRequested, this, &BattlePage::shopRequested);
    connect(hud, &GameHud::equipmentRequested, this, &BattlePage::equipmentRequested);
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
        clearBondSummary();
        emit battlePreparationModeChanged(false);
        syncFromBattleSystem();
        battleTimer->start();
    });
    connect(battleSystem, &battlesystem::battleEnded, this, [this](bool pcTeamAllDead) {
        battleTimer->stop();
        beginSettlement(!pcTeamAllDead);
    });
}

std::vector<CharacterPlacement> BattlePage::preparedPlacements() const {
    std::vector<CharacterPlacement> placements = game->preparedPlacements();
    for (CharacterPlacement& placement : placements) {
        if (!placement.source) {
            continue;
        }
        const auto uidIt = unitIdToOwnedCardUid.find(placement.source->id());
        if (uidIt == unitIdToOwnedCardUid.end()) {
            continue;
        }
        const int cardUid = uidIt->second;
        for (const OwnedEquipment& equipment : currentState.ownedEquipment) {
            if (equipment.equippedCardUid == cardUid) {
                placement.hasEquipment = true;
                placement.equipmentGroup = equipment.group;
                placement.equipmentId = equipment.equipmentId;
                break;
            }
        }
    }
    return placements;
}

void BattlePage::setState(const GameState& state) {
    currentState = state;
    hud->setState(state);
    if (game) {
        game->setDeploymentLimit(deploymentLimitForCurrentLayer());
    }
    syncRosterFromState();
    updateBondSummary();
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
    ensureBondSummary();
    updateBondSummary();
    emit battlePreparationModeChanged(true);
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

    game->clearRoster();
    roster.clear();
    unitIdToOwnedCardUid.clear();
    rosterSignature = currentState.ownedCharacterCards;
    for (const OwnedCharacterCard& card : currentState.ownedCharacterCards) {
        std::unique_ptr<Character> character = characterfactory::create(card.templateId);
        if (!character) {
            continue;
        }
        applyStarScaling(character.get(), card.starLevel);
        Character* raw = character.get();
        roster.push_back(std::move(character));
        Unit* unit = game->addCharacter(raw, raw->name());
        if (unit) {
            unitIdToOwnedCardUid[unit->id()] = card.uid;
        }
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
        return 6;
    }
    if (currentState.currentLayerId == 2) {
        return 7;
    }
    return 8;
}

void BattlePage::syncFromBattleSystem() {
    if (battleSystem && !settlementActive) {
        game->syncFromBattleSystem(*battleSystem);
    }
}

void BattlePage::ensureBondSummary() {
    if (bondSummaryLabel || !view) {
        return;
    }
    bondSummaryLabel = new QLabel(view);
    bondSummaryLabel->setTextFormat(Qt::RichText);
    bondSummaryLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    bondSummaryLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    bondSummaryLabel->setStyleSheet("background: rgba(248, 241, 225, 232); border: 2px solid rgba(32, 28, 24, 210); border-radius: 8px; padding: 10px 12px;");
    bondSummaryLabel->setMinimumWidth(190);
    bondSummaryLabel->raise();
}

void BattlePage::clearBondSummary() {
    if (!bondSummaryLabel) {
        return;
    }
    bondSummaryLabel->hide();
    bondSummaryLabel->deleteLater();
    bondSummaryLabel = nullptr;
}

void BattlePage::updateBondSummary() {
    if (!game || (battleSystem && battleSystem->isRunning()) || settlementActive) {
        clearBondSummary();
        return;
    }

    const std::unordered_map<Bond, int> counts = battlesystem::count_bond(game->preparedPlacements());
    if (counts.empty()) {
        if (bondSummaryLabel) {
            bondSummaryLabel->hide();
        }
        return;
    }

    ensureBondSummary();
    if (!bondSummaryLabel) {
        return;
    }

    const std::vector<Bond> displayOrder = {
        Bond::soldier,
        Bond::mage,
        Bond::building,
        Bond::witch,
        Bond::assassin,
        Bond::pastor,
        Bond::animal,
        Bond::beast,
        Bond::chess
    };

    QStringList rows;
    for (Bond bond : displayOrder) {
        const auto it = counts.find(bond);
        if (it == counts.end() || it->second <= 0) {
            continue;
        }

        QString row = QString("<div style=\"color:%1; font-weight:800; font-size:15px; margin-top:5px;\">%2 "
                              "<span style=\"font-weight:900;\">%3</span></div>")
                          .arg(normalBondColor, bondDisplayName(bond))
                          .arg(it->second);
        const std::vector<BondEffectText> effects = bondEffectTexts(bond);
        if (effects.empty()) {
            row += QString("<div style=\"color:%1; font-size:13px; margin-left:10px;\">暂无羁绊效果</div>")
                       .arg(normalBondColor);
        } else {
            for (const BondEffectText& effect : effects) {
                row += QString("<div style=\"color:%1; font-size:13px; margin-left:10px;\">%2 "
                               "<span style=\"color:%1;\">%3</span></div>")
                           .arg(normalBondColor,
                                coloredBondNumber(effect.threshold, it->second >= effect.threshold),
                                QString::fromUtf8(effect.description));
            }
        }
        rows.push_back(row);
    }

    if (rows.empty()) {
        bondSummaryLabel->hide();
        return;
    }

    bondSummaryLabel->setText(rows.join(""));
    bondSummaryLabel->adjustSize();
    bondSummaryLabel->show();
    bondSummaryLabel->raise();
    positionSettlementButton();
}

bool BattlePage::eventFilter(QObject* watched, QEvent* event) {
    if (watched == view && event->type() == QEvent::Resize) {
        fitSceneInView();
    }
    if (watched == view->viewport()) {
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
            auto* dragEvent = static_cast<QDragMoveEvent*>(event);
            if (isEquipmentDropAllowed() && dragEvent->mimeData()->hasFormat(equipmentMimeType)) {
                dragEvent->acceptProposedAction();
                return true;
            }
        }
        if (event->type() == QEvent::Drop) {
            auto* dropEvent = static_cast<QDropEvent*>(event);
            if (!isEquipmentDropAllowed() || !dropEvent->mimeData()->hasFormat(equipmentMimeType)) {
                return QWidget::eventFilter(watched, event);
            }
            const int equipmentInstanceId = dropEvent->mimeData()->data(equipmentMimeType).toInt();
            const QPointF scenePos = view->mapToScene(dropEvent->position().toPoint());
            const int unitId = game ? game->pcRosterUnitIdAtScenePos(scenePos) : -1;
            const auto uidIt = unitIdToOwnedCardUid.find(unitId);
            if (uidIt == unitIdToOwnedCardUid.end()) {
                return QWidget::eventFilter(watched, event);
            }
            const int ownedCardUid = uidIt->second;
            const bool alreadyEquipped =
                std::any_of(currentState.ownedEquipment.begin(), currentState.ownedEquipment.end(),
                            [ownedCardUid](const OwnedEquipment& equipment) {
                                return equipment.equippedCardUid == ownedCardUid;
                            });
            if (alreadyEquipped) {
                showEquipmentWarning();
                dropEvent->acceptProposedAction();
                return true;
            }
            emit equipEquipmentRequested(equipmentInstanceId, ownedCardUid);
            dropEvent->acceptProposedAction();
            return true;
        }
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
    if (equipmentWarningLabel) {
        equipmentWarningLabel->move((view->viewport()->width() - equipmentWarningLabel->width()) / 2,
                                    (view->viewport()->height() - equipmentWarningLabel->height()) / 2);
    }
    if (zoneLegend) {
        const int legendX = view->viewport()->width() - zoneLegend->width() - 34;
        const int legendY = std::clamp(static_cast<int>(view->viewport()->height() * 0.52),
                                       96,
                                       std::max(96, view->viewport()->height() - zoneLegend->height() - 170));
        zoneLegend->move(qMax(0, legendX), legendY);
        zoneLegend->raise();
    }
    if (bondSummaryLabel && bondSummaryLabel->isVisible()) {
        const int bondX = view->viewport()->width() - bondSummaryLabel->width() - 34;
        bondSummaryLabel->move(qMax(0, bondX), 34);
        bondSummaryLabel->raise();
    }
}

bool BattlePage::isEquipmentDropAllowed() const {
    return game &&
           game->preparationActive() &&
           (!battleSystem || !battleSystem->isRunning()) &&
           !settlementActive;
}

void BattlePage::showEquipmentWarning() {
    if (!equipmentWarningLabel || !view) {
        return;
    }
    positionSettlementButton();
    auto* effect = new QGraphicsOpacityEffect(equipmentWarningLabel);
    effect->setOpacity(1.0);
    equipmentWarningLabel->setGraphicsEffect(effect);
    equipmentWarningLabel->show();
    equipmentWarningLabel->raise();
    QTimer::singleShot(3000, this, [this]() {
        if (!equipmentWarningLabel) {
            return;
        }
        auto* effect = qobject_cast<QGraphicsOpacityEffect*>(equipmentWarningLabel->graphicsEffect());
        if (!effect) {
            equipmentWarningLabel->hide();
            return;
        }
        auto* fade = new QPropertyAnimation(effect, "opacity", equipmentWarningLabel);
        fade->setDuration(520);
        fade->setStartValue(1.0);
        fade->setEndValue(0.0);
        connect(fade, &QPropertyAnimation::finished, equipmentWarningLabel, &QLabel::hide);
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void BattlePage::beginSettlement(bool victory) {
    settlementActive = true;
    pendingBattleResult = {victory};
    startButton->hide();
    chestSettlementPending = false;
    pendingRewardEquipmentClaims = 0;
    pendingRewardEquipmentDrops.clear();
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
    for (const BattleScene::EquipmentRewardVisual& drop : pendingRewardEquipmentDrops) {
        emit battleChestEquipmentClaimed(drop.group, drop.equipmentId);
    }
    pendingRewardEquipmentDrops.clear();
    pendingRewardEquipmentClaims = 0;
    settlementButton->hide();
    settlementActive = false;
    chestSettlementPending = false;
    emit battleFinished(pendingBattleResult);
}

void BattlePage::completeVictoryChest() {
    if (!chestSettlementPending) {
        return;
    }
    const int gold = victoryChestGold();
    const std::vector<BattleScene::EquipmentRewardVisual> drops = rollVictoryEquipmentDrops();
    pendingRewardEquipmentDrops = drops;
    pendingRewardEquipmentClaims = static_cast<int>(pendingRewardEquipmentDrops.size());
    game->playRewardEffects(gold, drops);
    emit battleChestOpened(gold);
    QTimer::singleShot(pendingRewardEquipmentClaims > 0 ? 1500 : 900, this, [this]() {
        if (chestSettlementPending) {
            completeSettlement();
        }
    });
}

int BattlePage::victoryChestGold() const {
    int gold = 2 + currentState.currentLayerId;
    if (currentState.currentBattle.kind == BattleKind::Elite) {
        gold *= 2;
    }
    gold += extraChestGoldFromHexTech();
    const int bonusPercent = rewardModifierPercent("gold_drop_percent");
    if (bonusPercent > 0) {
        gold += static_cast<int>(std::lround(gold * bonusPercent / 100.0));
    }
    return gold;
}

int BattlePage::rewardModifierPercent(const std::string& modifierKey) const {
    int total = 0;
    HexTechRepository repository;
    for (const HexTechCardRecord& record : currentState.selectedHexTechCards) {
        const auto definition = repository.findById(record.hexTechId);
        if (!definition) {
            continue;
        }
        for (const HexTechEffect& effect : definition->effects) {
            if (effect.type == "reward_modifier" && effect.modifierKey == modifierKey) {
                total += effect.value;
            }
        }
    }
    return total;
}

std::vector<BattleScene::EquipmentRewardVisual> BattlePage::rollVictoryEquipmentDrops() const {
    std::vector<BattleScene::EquipmentRewardVisual> drops;
    if (currentState.currentBattle.kind != BattleKind::Normal &&
        currentState.currentBattle.kind != BattleKind::Elite) {
        return drops;
    }

    EquipmentRepository repository;
    auto makeVisual = [](const Equipment& equipment) {
        BattleScene::EquipmentRewardVisual visual;
        visual.group = equipment.group();
        visual.equipmentId = equipment.id();
        visual.iconPath = equipment.iconPath();
        return visual;
    };

    std::mt19937 rng(static_cast<unsigned int>(currentState.seed * 2654435761u +
                                               currentState.currentLayerId * 1103515245u +
                                               currentState.currentNodeId * 4099u +
                                               currentState.ownedEquipment.size() * 97u + 73u));
    std::uniform_int_distribution<int> roll(1, 100);

    int basicChance = currentState.currentBattle.kind == BattleKind::Elite ? 50 : 30;
    int advancedChance = currentState.currentBattle.kind == BattleKind::Elite ? 10 : 0;
    basicChance += rewardModifierPercent("equipment_drop_percent");
    advancedChance += rewardModifierPercent("rare_drop_percent");
    if (currentState.currentBattle.kind == BattleKind::Elite) {
        const int eliteBonus = rewardModifierPercent("elite_reward_percent");
        basicChance += eliteBonus;
        advancedChance += eliteBonus;
    }
    basicChance = std::clamp(basicChance, 0, 100);
    advancedChance = std::clamp(advancedChance, 0, 100);

    if (roll(rng) <= basicChance) {
        const std::vector<Equipment>& pool = repository.allBasic();
        if (!pool.empty()) {
            std::uniform_int_distribution<int> pick(0, static_cast<int>(pool.size()) - 1);
            drops.push_back(makeVisual(pool[static_cast<size_t>(pick(rng))]));
        }
    }

    if (advancedChance > 0 && roll(rng) <= advancedChance) {
        const std::vector<Equipment>& pool = repository.allAdvanced();
        if (!pool.empty()) {
            std::uniform_int_distribution<int> pick(0, static_cast<int>(pool.size()) - 1);
            drops.push_back(makeVisual(pool[static_cast<size_t>(pick(rng))]));
        }
    }
    return drops;
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

