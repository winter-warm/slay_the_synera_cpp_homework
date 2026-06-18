#include "battlescene.h"
#include "entity/character/character.h"
#include "entity/object/object.h"
#include "entity/unit.h"
#include "gui/board/benchslotitem.h"
#include "gui/board/griditem.h"
#include "gui/board/hexlayout.h"
#include "gui/board/objectitem.h"
#include "gui/board/unititem.h"
#include <QBrush>
#include <QEasingCurve>
#include <QFont>
#include <QGraphicsColorizeEffect>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QParallelAnimationGroup>
#include <QPainter>
#include <QPointer>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QVariantAnimation>
#include <algorithm>
#include <cmath>
#include <random>


static constexpr qreal zGrid = 0.0, zUnit = 1.0;
static constexpr qreal zBench = 4.0, zBenchUnit = 5.0, zDraggingUnit = 6.0, zVisualEffect = 7.0;
static constexpr qreal sceneW = 1280.0, sceneH = 720.0, scenePad = 40.0;
static constexpr qreal pi = 3.14159265358979323846;

class SquareParticle : public QGraphicsObject {
public:
    SquareParticle(const QColor& color, qreal size, QGraphicsItem* parent = nullptr)
        : QGraphicsObject(parent)
        , particleColor(color)
        , particleSize(size)
    {}

    QRectF boundingRect() const override {
        return QRectF(-particleSize * 0.5, -particleSize * 0.5, particleSize, particleSize);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        if (!painter) {
            return;
        }
        painter->setPen(Qt::NoPen);
        painter->setBrush(particleColor);
        painter->drawRect(boundingRect());
    }

private:
    QColor particleColor;
    qreal particleSize;
};

static QColor colorForZone(Board::Zone zone) {
    switch (zone) {
    case Board::Zone::Player:
        return QColor(60, 60, 80);
    case Board::Zone::Enemy:
        return QColor(80, 60, 60);
    case Board::Zone::Neutral:
        return QColor(180, 160, 105);
    case Board::Zone::BlockedObstacle:
        return QColor(70, 125, 75);
    case Board::Zone::OpenObstacle:
        return QColor(130, 105, 40);
    default:
        return QColor(65, 65, 65);
    }
}

BattleScene::BattleScene(QObject* parent)
    : QObject(parent)
    , boardRef(nullptr)
    , layout(std::make_unique<HexLayout>())
    , sceneObj(new QGraphicsScene(this))
    , deploymentTextItem(nullptr)
    , rewardChestItem(nullptr)
    , dragActive(false)
    , activeUnitId(-1)
    , benchRows(2)
    , benchCols(12)
    , benchSize(78.0)
    , benchGap(10.0)
    , benchOrigin(0.0, 0.0)
    , battlePhase(BattlePhase::Prep)
    , deploymentLimit(4) {}

BattleScene::~BattleScene() {
    units.clear();
}

void BattleScene::initialize() {
    buildScene();
}

void BattleScene::setBoard(Board* board) {
    clearRewardChest();
    boardRef = board;
    battlePhase = BattlePhase::Prep;
    battleDisplayUnits.clear();
    buildScene();
    resetPreparation();
}

void BattleScene::resetPreparation() {
    battlePhase = BattlePhase::Prep;
    places.clear();

    for (Unit* unit : units) {
        if (!unit) {
            continue;
        }

        const QPoint slot = firstFreeBenchSlot();
        if (slot.x() < 0) {
            places[unit->id()] = {};
        } else {
            Placement place;
            place.area = UnitArea::Bench;
            place.slot = slot;
            places[unit->id()] = place;
        }
    }
    hideBench(false);
    syncFromState();
}

void BattleScene::clearRoster() {
    clearRewardChest();
    Board* board = activeBoard();
    for (Unit* unit : units) {
        if (unit && board) {
            board->remove(unit);
        }
        if (unit) {
            removeCharacterItem(unit->id());
        }
    }
    units.clear();
    places.clear();
    syncFromState();
}

void BattleScene::setDeploymentLimit(int limit) {
    deploymentLimit = std::max(1, limit);
    updateDeploymentText();
}

bool BattleScene::spawnRewardChest(int seedSalt) {
    Board* board = activeBoard();
    if (!board) {
        return false;
    }

    clearRewardChest();

    auto collect = [board](auto predicate) {
        std::vector<Hex> out;
        for (const Hex& hex : board->cells()) {
            if (board->empty(hex) && predicate(board->zone(hex))) {
                out.push_back(hex);
            }
        }
        return out;
    };

    std::vector<Hex> candidates = collect([](Board::Zone zone) {
        return zone == Board::Zone::Enemy;
    });
    if (candidates.empty()) {
        candidates = collect([](Board::Zone zone) {
            return zone != Board::Zone::Player && zone != Board::Zone::None;
        });
    }
    if (candidates.empty()) {
        candidates = collect([](Board::Zone) {
            return true;
        });
    }
    if (candidates.empty()) {
        return false;
    }

    std::mt19937 rng(static_cast<unsigned int>(seedSalt));
    std::uniform_int_distribution<int> pick(0, static_cast<int>(candidates.size()) - 1);
    const Hex hex = candidates[static_cast<size_t>(pick(rng))];

    rewardChest = std::make_unique<Object>("reward_chest",
                                           "assets/ui/battle_chest_closed.png",
                                           "assets/ui/battle_chest_open.png");
    Object* chest = rewardChest.get();
    if (!board->add(chest, hex)) {
        rewardChest.reset();
        return false;
    }

    createObjectItem(chest);
    if (rewardChestItem) {
        rewardChestItem->setHex(hex);
        rewardChestItem->setPos(layout->toWorld(hex));
    }
    return true;
}

void BattleScene::setBattleCharacters(const std::vector<Character*>& characters) {
    battleDisplayUnits = characters;
    battlePhase = BattlePhase::Battle;
    phase = 1;
    hideBench(true);
    clearTransientUnitItems();

    for (Character* character : battleDisplayUnits) {
        createUnitItem(character);
    }
    syncFromState();
}

void BattleScene::removeCharacterItem(int id) {
    auto itemIt = unitItemMap.find(id);
    if (itemIt == unitItemMap.end()) {
        return;
    }

    UnitItem* item = itemIt->second;
    unitItemMap.erase(itemIt);
    auto vecIt = std::find(unitItems.begin(), unitItems.end(), item);
    if (vecIt != unitItems.end()) {
        unitItems.erase(vecIt);
    }
    sceneObj->removeItem(item);
    delete item;
}

void BattleScene::syncFromBattleSystem(const battlesystem& battleSystem) {
    if (battleSystem.isRunning()) {
        setBattleCharacters(battleSystem.liveCharacters());
        return;
    }

    battleDisplayUnits.clear();
    battlePhase = BattlePhase::Prep;
    phase = 0;
    clearTransientUnitItems();
    hideBench(false);
    syncFromState();
}

std::vector<CharacterPlacement> BattleScene::preparedPlacements() const {
    std::vector<CharacterPlacement> out;
    for (const auto& entry : places) {
        const Placement& place = entry.second;
        if (place.area != UnitArea::Board || !place.hasHex) {
            continue;
        }

        Character* character = dynamic_cast<Character*>(findUnitById(entry.first));
        if (character) {
            CharacterPlacement placement;
            placement.source = character;
            placement.hex = place.hex;
            out.push_back(placement);
        }
    }
    return out;
}

Unit* BattleScene::addCharacter(Character* character, const std::string& displayName) {
    if (!character) {
        return nullptr;
    }

    Unit* unit = character;
    unit->setName(displayName);
    units.push_back(unit);

    const QPoint slot = firstFreeBenchSlot();
    if (slot.x() < 0) {
        places[unit->id()] = {};
    } else {
        Placement place;
        place.area = UnitArea::Bench;
        place.slot = slot;
        places[unit->id()] = place;
    }

    if (!gridItems.empty()) {
        createUnitItem(unit);
        syncFromState();
    }

    return unit;
}

void BattleScene::handleDragStarted(int unitId, const QPointF&) {
    const auto placementIt = places.find(unitId);
    if (!canDragUnit(unitId) ||
        placementIt == places.end() ||
        placementIt->second.area == UnitArea::Hidden) {
        dragActive = false;
        activeUnitId = -1;
        return;
    }

    dragActive = true;
    activeUnitId = unitId;

    UnitItem* item = findUnitItem(unitId);
    if (item) {
        item->setZValue(zDraggingUnit);
    }
}

void BattleScene::handleDragMoved(int unitId, const QPointF& scenePos) {
    if (!dragActive || unitId != activeUnitId) {
        return;
    }

    clearHighlights();

    UnitItem* item = findUnitItem(unitId);
    if (item) {
        item->setPos(scenePos);
    }

    const DropTarget target = dropTargetAt(scenePos);
    if (!target.valid) {
        return;
    }

    if (target.area == UnitArea::Board) {
        GridItem* targetItem = target.hasHex ? findGridItem(target.hex) : nullptr;
        if (targetItem) {
            targetItem->setHoverActive(true);
            targetItem->setDropActive(canApplyDrop(unitId, target));
        }
    } else if (target.area == UnitArea::Bench) {
        BenchSlotItem* targetItem = findBenchSlotItem(target.slot);
        if (targetItem) {
            targetItem->setHoverActive(true);
            targetItem->setDropActive(canApplyDrop(unitId, target));
        }
    }
}

void BattleScene::handleDropCommand(int unitId, const QPointF& scenePos) {
    if (!dragActive || unitId != activeUnitId) {
        return;
    }

    const DropTarget target = dropTargetAt(scenePos);

    clearHighlights();
    if (target.valid && canApplyDrop(unitId, target)) {
        applyDrop(unitId, target);
    }

    UnitItem* item = findUnitItem(activeUnitId);
    if (item) {
        item->setZValue(zUnit);
    }

    dragActive = false;
    activeUnitId = -1;

    syncFromState();
}

Unit* BattleScene::findUnitById(int unitId) const {
    for (Unit* unit : units) {
        if (unit && unit->id() == unitId) {
            return unit;
        }
    }
    return nullptr;
}

bool BattleScene::isPreparationPhase() const {
    return battlePhase == BattlePhase::Prep;
}

bool BattleScene::canDragUnit(int unitId) const {
    Unit* unit = findUnitById(unitId);
    Character* character = dynamic_cast<Character*>(unit);
    return isPreparationPhase() && character && character->getteam().getteam() == teams::pc;
}

GridItem* BattleScene::findGridItem(const Hex& hex) const {
    for (GridItem* item : gridItems) {
        if (item && item->hex() == hex) {
            return item;
        }
    }
    return nullptr;
}

BenchSlotItem* BattleScene::findBenchSlotItem(const QPoint& slotPos) const {
    for (BenchSlotItem* item : benchItems) {
        if (item && item->slotPos() == slotPos) {
            return item;
        }
    }
    return nullptr;
}

BenchSlotItem* BattleScene::findBenchSlotItemAt(const QPointF& scenePos) const {
    for (BenchSlotItem* item : benchItems) {
        if (!item) {
            continue;
        }

        const QPointF localPos = item->mapFromScene(scenePos);
        if (item->boundingRect().contains(localPos)) {
            return item;
        }
    }
    return nullptr;
}

UnitItem* BattleScene::findUnitItem(int unitId) const {
    auto it = unitItemMap.find(unitId);
    if (it == unitItemMap.end()) {
        return nullptr;
    }
    return it->second;
}

bool BattleScene::isRosterUnit(Unit* unit) const {
    return std::find(units.begin(), units.end(), unit) != units.end();
}

bool BattleScene::isBattleDisplayUnit(Unit* unit) const {
    for (Character* battleUnit : battleDisplayUnits) {
        if (battleUnit == unit) {
            return true;
        }
    }
    return false;
}

bool BattleScene::isRenderedBattleId(int unitId) const {
    for (Character* character : battleDisplayUnits) {
        if (character && character->id() == unitId) {
            return true;
        }
    }
    return false;
}

void BattleScene::clearTransientUnitItems() {
    Board* board = activeBoard();
    std::vector<int> removeIds;
    for (const auto& entry : unitItemMap) {
        UnitItem* item = entry.second;
        Hex ignored;
        const bool onBoard = board && item && board->posOf(item->unit(), &ignored);
        if (!item || (!isRosterUnit(item->unit()) && !onBoard)) {
            removeIds.push_back(entry.first);
        }
    }
    for (int id : removeIds) {
        removeCharacterItem(id);
    }
}

void BattleScene::hideBench(bool hidden) {
    for (BenchSlotItem* item : benchItems) {
        if (item) {
            item->setVisible(!hidden);
        }
    }
}

Board* BattleScene::activeBoard() const {
    return boardRef;
}

QRectF BattleScene::rawBoardBounds() const {
    QRectF bounds;
    bool first = true;
    Board* board = activeBoard();
    if (!board) {
        return bounds;
    }

    for (const Hex& hex : board->cells()) {
        const QRectF rect = layout->poly(hex).boundingRect();
        bounds = first ? rect : bounds.united(rect);
        first = false;
    }
    return bounds;
}

void BattleScene::clearHighlights() {
    for (GridItem* item : gridItems) {
        if (item) {
            item->setHoverActive(false);
            item->setDropActive(false);
        }
    }

    for (BenchSlotItem* item : benchItems) {
        if (item) {
            item->setHoverActive(false);
            item->setDropActive(false);
        }
    }
}

bool BattleScene::canApplyDrop(int unitId, const DropTarget& target) const {
    Board* board = activeBoard();
    Unit* unit = findUnitById(unitId);
    if (!board || !unit || !canDragUnit(unitId) || !target.valid) {
        return false;
    }

    const auto placementIt = places.find(unitId);
    if (placementIt == places.end() || placementIt->second.area == UnitArea::Hidden) {
        return false;
    }

    const Placement& source = placementIt->second;

    if (target.area == UnitArea::Board) {
        const Board::Zone z = target.hasHex ? board->zone(target.hex) : Board::Zone::None;
        if (!target.hasHex ||
            !board->has(target.hex) ||
            !board->empty(target.hex) ||
            (z != Board::Zone::Player && z != Board::Zone::Neutral)) {
            return false;
        }
        if (source.area == UnitArea::Board) {
            return source.hasHex && board->unitAt(source.hex) == unit && source.hex != target.hex;
        }
        const Unit* sameNameOnBoard = boardRosterUnitWithName(unit->name(), unitId);
        const int projectedCount = boardRosterCount() + (sameNameOnBoard ? 0 : 1);
        if (projectedCount > deploymentLimit) {
            return false;
        }
        return source.area == UnitArea::Bench;
    }

    if (target.area == UnitArea::Bench) {
        if (target.slot.x() < 0 || target.slot.x() >= benchCols ||
            target.slot.y() < 0 || target.slot.y() >= benchRows) {
            return false;
        }
        if (!isBenchSlotFree(target.slot, unitId)) {
            return false;
        }
        return source.area == UnitArea::Board || source.slot != target.slot;
    }

    return false;
}

void BattleScene::applyDrop(int unitId, const DropTarget& target) {
    Board* board = activeBoard();
    Unit* unit = findUnitById(unitId);
    if (!board || !unit) {
        return;
    }

    const auto placementIt = places.find(unitId);
    if (placementIt != places.end() && placementIt->second.area == UnitArea::Board) {
        board->remove(unit);
    }

    if (target.area == UnitArea::Board) {
        if (Unit* sameNameOnBoard = boardRosterUnitWithName(unit->name(), unitId)) {
            moveUnitToBench(sameNameOnBoard);
        }
        if (target.hasHex) {
            board->add(unit, target.hex);
        }
    } else {
        unit->clearPosition();
    }

    Placement place;
    place.area = target.area;
    place.hex = target.hex;
    place.hasHex = target.hasHex;
    place.slot = target.slot;
    places[unitId] = place;
    updateDeploymentText();
}

int BattleScene::boardRosterCount() const {
    int count = 0;
    for (const auto& entry : places) {
        if (entry.second.area != UnitArea::Board || !entry.second.hasHex) {
            continue;
        }
        Unit* unit = findUnitById(entry.first);
        Character* character = dynamic_cast<Character*>(unit);
        if (character && character->getteam().getteam() == teams::pc) {
            ++count;
        }
    }
    return count;
}

Unit* BattleScene::boardRosterUnitWithName(const std::string& name, int ignoredUnitId) const {
    for (const auto& entry : places) {
        if (entry.first == ignoredUnitId ||
            entry.second.area != UnitArea::Board ||
            !entry.second.hasHex) {
            continue;
        }
        Unit* unit = findUnitById(entry.first);
        Character* character = dynamic_cast<Character*>(unit);
        if (character && character->getteam().getteam() == teams::pc && unit->name() == name) {
            return unit;
        }
    }
    return nullptr;
}

void BattleScene::moveUnitToBench(Unit* unit) {
    if (!unit) {
        return;
    }
    Board* board = activeBoard();
    if (board) {
        board->remove(unit);
    }
    const QPoint slot = firstFreeBenchSlot();
    Placement place;
    if (slot.x() >= 0) {
        place.area = UnitArea::Bench;
        place.slot = slot;
    }
    places[unit->id()] = place;
}

void BattleScene::updateDeploymentText() {
    if (!deploymentTextItem) {
        return;
    }
    deploymentTextItem->setPlainText(QString("%1/%2").arg(boardRosterCount()).arg(deploymentLimit));
    deploymentTextItem->setVisible(isPreparationPhase());
    QRectF bounds = rawBoardBounds();
    if (bounds.isEmpty()) {
        bounds = QRectF(scenePad, scenePad, sceneW - scenePad * 2.0, benchOrigin.y() - scenePad * 2.0);
    }
    const QRectF textBounds = deploymentTextItem->boundingRect();
    deploymentTextItem->setPos(bounds.center().x() - textBounds.width() * 0.5,
                               bounds.center().y() - textBounds.height() * 0.5);
}

void BattleScene::consumeVisualEvents(UnitItem* item, Character* character) {
    if (!item || !character || battlePhase != BattlePhase::Battle) {
        return;
    }

    if (character->getrender().takeHitFlash()) {
        playHitFlash(item);
    }

    const std::vector<int> targets = character->getrender().takeAttackLungeTargets();
    for (int targetId : targets) {
        playAttackLunge(item, findUnitItem(targetId));
    }

    if (character->getrender().takeSkillBurst()) {
        playSkillBurst(item);
    }
}

void BattleScene::playHitFlash(UnitItem* item) {
    if (!item) {
        return;
    }

    auto* effect = new QGraphicsColorizeEffect(item);
    effect->setColor(QColor(255, 35, 35));
    effect->setStrength(0.85);
    item->setGraphicsEffect(effect);

    auto* fade = new QPropertyAnimation(effect, "strength", effect);
    fade->setDuration(260);
    fade->setStartValue(0.85);
    fade->setEndValue(0.0);
    fade->setEasingCurve(QEasingCurve::OutCubic);
    QPointer<UnitItem> guardedItem(item);
    QPointer<QGraphicsColorizeEffect> guardedEffect(effect);
    connect(fade, &QPropertyAnimation::finished, this, [guardedItem, guardedEffect]() {
        if (guardedItem && guardedEffect && guardedItem->graphicsEffect() == guardedEffect) {
            guardedEffect->setEnabled(false);
        }
    });
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}

void BattleScene::playAttackLunge(UnitItem* item, UnitItem* targetItem) {
    if (!item || !targetItem || item == targetItem) {
        return;
    }

    const QPointF start = item->pos();
    QPointF direction = targetItem->pos() - start;
    const qreal length = std::hypot(direction.x(), direction.y());
    if (length <= 0.001) {
        return;
    }
    direction /= length;
    const QPointF peak = start + direction * std::min<qreal>(length * 0.5, 34.0);

    QPointer<UnitItem> guardedItem(item);
    auto* group = new QSequentialAnimationGroup(this);
    auto* out = new QPropertyAnimation(guardedItem, "pos", group);
    out->setDuration(70);
    out->setStartValue(start);
    out->setEndValue(peak);
    out->setEasingCurve(QEasingCurve::OutQuad);

    auto* back = new QPropertyAnimation(guardedItem, "pos", group);
    back->setDuration(110);
    back->setStartValue(peak);
    back->setEndValue(start);
    back->setEasingCurve(QEasingCurve::InOutQuad);

    group->addAnimation(out);
    group->addAnimation(back);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void BattleScene::playSkillBurst(UnitItem* item) {
    if (!item || !sceneObj) {
        return;
    }

    const QPointF center = item->pos();
    const QColor colors[] = {
        QColor(255, 64, 96),
        QColor(255, 196, 48),
        QColor(80, 220, 140),
        QColor(64, 152, 255),
        QColor(190, 92, 255),
        QColor(255, 120, 48)
    };

    auto* group = new QParallelAnimationGroup(this);
    constexpr int particleCount = 28;
    for (int i = 0; i < particleCount; ++i) {
        const qreal angle = (2.0 * pi * i / particleCount) + ((i % 3) - 1) * 0.12;
        const qreal distance = 38.0 + (i % 5) * 8.0;
        const qreal size = 5.0 + (i % 4);
        const QPointF end(center.x() + std::cos(angle) * distance,
                          center.y() + std::sin(angle) * distance);

        auto* particle = new SquareParticle(colors[i % 6], size);
        particle->setPos(center);
        particle->setOpacity(0.95);
        particle->setRotation(i * 17);
        particle->setZValue(zVisualEffect);
        sceneObj->addItem(particle);

        QPointer<SquareParticle> guardedParticle(particle);
        auto* move = new QVariantAnimation(group);
        move->setDuration(1500);
        move->setStartValue(center);
        move->setEndValue(end);
        move->setEasingCurve(QEasingCurve::OutCubic);
        connect(move, &QVariantAnimation::valueChanged, this, [guardedParticle](const QVariant& value) {
            if (guardedParticle) {
                guardedParticle->setPos(value.toPointF());
            }
        });

        auto* fade = new QVariantAnimation(group);
        fade->setDuration(1500);
        fade->setStartValue(0.95);
        fade->setEndValue(0.0);
        fade->setEasingCurve(QEasingCurve::InQuad);
        connect(fade, &QVariantAnimation::valueChanged, this, [guardedParticle](const QVariant& value) {
            if (guardedParticle) {
                guardedParticle->setOpacity(value.toReal());
            }
        });

        group->addAnimation(move);
        group->addAnimation(fade);
    }

    connect(group, &QParallelAnimationGroup::finished, this, [this, group]() {
        const QList<QGraphicsItem*> items = sceneObj ? sceneObj->items() : QList<QGraphicsItem*>();
        for (QGraphicsItem* graphicsItem : items) {
            if (auto* particle = dynamic_cast<SquareParticle*>(graphicsItem)) {
                if (particle->opacity() <= 0.01) {
                    sceneObj->removeItem(particle);
                    delete particle;
                }
            }
        }
        group->deleteLater();
    });
    group->start();
}

void BattleScene::clearRewardChest() {
    if (rewardChest) {
        if (Board* board = activeBoard()) {
            board->remove(rewardChest.get());
        }
    }
    if (rewardChestItem) {
        sceneObj->removeItem(rewardChestItem);
        delete rewardChestItem;
        rewardChestItem = nullptr;
    }
    rewardChest.reset();
}

void BattleScene::createObjectItem(Object* object) {
    if (!object || rewardChestItem) {
        return;
    }

    rewardChestItem = new ObjectItem(object);
    rewardChestItem->setZValue(zUnit);
    sceneObj->addItem(rewardChestItem);
    connect(rewardChestItem, &ObjectItem::opened,
            this, &BattleScene::handleObjectOpened);
}

void BattleScene::removeObjectItem(int id) {
    if (!rewardChestItem || rewardChestItem->objectId() != id) {
        return;
    }
    sceneObj->removeItem(rewardChestItem);
    delete rewardChestItem;
    rewardChestItem = nullptr;
}

void BattleScene::handleObjectOpened(int objectId) {
    if (!rewardChestItem || rewardChestItem->objectId() != objectId) {
        return;
    }
    const QPointF center = rewardChestItem->pos();
    playChestRewardBurst(center);
    QTimer::singleShot(800, this, [this]() {
        emit rewardChestOpened();
    });
}

void BattleScene::playChestRewardBurst(const QPointF& center) {
    if (!sceneObj) {
        return;
    }

    auto* group = new QParallelAnimationGroup(this);
    constexpr int particleCount = 18;
    for (int i = 0; i < particleCount; ++i) {
        const qreal angle = 2.0 * pi * i / particleCount;
        const qreal distance = 26.0 + (i % 4) * 8.0;
        const QPointF start(center.x(), center.y() - 18.0);
        const QPointF end(start.x() + std::cos(angle) * distance,
                          start.y() + std::sin(angle) * distance - 12.0);

        auto* particle = new SquareParticle(QColor(255, 205, 55), 6.0);
        particle->setPos(start);
        particle->setOpacity(0.95);
        particle->setZValue(zVisualEffect);
        sceneObj->addItem(particle);

        QPointer<SquareParticle> guardedParticle(particle);
        auto* move = new QVariantAnimation(group);
        move->setDuration(760);
        move->setStartValue(start);
        move->setEndValue(end);
        move->setEasingCurve(QEasingCurve::OutCubic);
        connect(move, &QVariantAnimation::valueChanged, this, [guardedParticle](const QVariant& value) {
            if (guardedParticle) {
                guardedParticle->setPos(value.toPointF());
            }
        });

        auto* fade = new QVariantAnimation(group);
        fade->setDuration(760);
        fade->setStartValue(0.95);
        fade->setEndValue(0.0);
        connect(fade, &QVariantAnimation::valueChanged, this, [guardedParticle](const QVariant& value) {
            if (guardedParticle) {
                guardedParticle->setOpacity(value.toReal());
            }
        });

        group->addAnimation(move);
        group->addAnimation(fade);
    }

    connect(group, &QParallelAnimationGroup::finished, this, [this, group]() {
        const QList<QGraphicsItem*> items = sceneObj ? sceneObj->items() : QList<QGraphicsItem*>();
        for (QGraphicsItem* graphicsItem : items) {
            if (auto* particle = dynamic_cast<SquareParticle*>(graphicsItem)) {
                if (particle->opacity() <= 0.01) {
                    sceneObj->removeItem(particle);
                    delete particle;
                }
            }
        }
        group->deleteLater();
    });
    group->start();
}

void BattleScene::buildScene() {
    sceneObj->clear();
    gridItems.clear();
    deploymentTextItem = nullptr;
    rewardChestItem = nullptr;
    benchItems.clear();
    unitItems.clear();
    unitItemMap.clear();
    Board* board = activeBoard();
    layout->setCells(board ? board->cells() : std::vector<Hex>{});
    layout->setOrigin(QPointF(0.0, 0.0));
    layout->setSize(46.0, 69.0);

    const qreal benchW = benchCols * benchSize + (benchCols - 1) * benchGap;
    const qreal benchH = benchRows * benchSize + (benchRows - 1) * benchGap;
    benchOrigin = QPointF((sceneW - benchW) * 0.5, sceneH - scenePad - benchH);

    QRectF rawBounds = rawBoardBounds();
    const qreal topW = sceneW - scenePad * 2.0;
    const qreal topH = benchOrigin.y() - scenePad * 2.0;
    if (!rawBounds.isEmpty()) {
        const qreal scale = std::min<qreal>(1.0, std::min(topW / rawBounds.width(), topH / rawBounds.height()));
        layout->setSize(46.0 * scale, 69.0 * scale);
        rawBounds = rawBoardBounds();
    }

    const qreal boardX = scenePad + qMax<qreal>(0.0, (topW - rawBounds.width()) * 0.5) - rawBounds.left();
    const qreal boardY = scenePad + qMax<qreal>(0.0, (topH - rawBounds.height()) * 0.5) - rawBounds.top();
    layout->setOrigin(QPointF(boardX, boardY));

    if (!board) {
        sceneObj->setSceneRect(QRectF(0.0, 0.0, sceneW, sceneH));
        return;
    }

    for (const Hex& hex : board->cells()) {
        const QPolygonF poly = layout->poly(hex);
        GridItem* gridItem = new GridItem(hex, poly);
        gridItem->setZValue(zGrid);
        gridItem->setBaseColor(colorForZone(board->zone(hex)));

        sceneObj->addItem(gridItem);
        gridItems.push_back(gridItem);
    }

    deploymentTextItem = sceneObj->addText("", QFont("Arial", 72, QFont::Black));
    deploymentTextItem->setDefaultTextColor(QColor(255, 255, 255, 128));
    deploymentTextItem->setZValue(0.55);
    deploymentTextItem->setAcceptedMouseButtons(Qt::NoButton);
    updateDeploymentText();

    const QRectF slotRect(0.0, 0.0, benchSize, benchSize);

    for (int row = 0; row < benchRows; ++row) {
        for (int col = 0; col < benchCols; ++col) {
            BenchSlotItem* slotItem = new BenchSlotItem(row, col, slotRect);
            slotItem->setZValue(zBench);
            slotItem->setPos(benchSlotTopLeft(row, col));
            sceneObj->addItem(slotItem);
            benchItems.push_back(slotItem);
        }
    }

    for (Unit* unit : units) {
        createUnitItem(unit);
    }
    for (Unit* unit : board->units()) {
        if (Object* object = dynamic_cast<Object*>(unit)) {
            createObjectItem(object);
            Hex h;
            if (rewardChestItem && board->posOf(object, &h)) {
                rewardChestItem->setHex(h);
                rewardChestItem->setPos(layout->toWorld(h));
            }
        } else if (unit && !isRosterUnit(unit)) {
            createUnitItem(unit);
        }
    }

    sceneObj->setSceneRect(QRectF(0.0, 0.0, sceneW, sceneH));
    hideBench(!isPreparationPhase());
}

void BattleScene::createUnitItem(Unit* unit) {
    if (!unit || dynamic_cast<Object*>(unit) || unitItemMap.find(unit->id()) != unitItemMap.end()) {
        return;
    }

    UnitItem* unitItem = new UnitItem(unit);
    unitItem->setZValue(zUnit);
    sceneObj->addItem(unitItem);
    unitItems.push_back(unitItem);
    unitItemMap[unit->id()] = unitItem;

    connect(unitItem, &UnitItem::dragStarted,
            this, &BattleScene::handleDragStarted);
    connect(unitItem, &UnitItem::dragMoved,
            this, &BattleScene::handleDragMoved);
    connect(unitItem, &UnitItem::dragDropped,
            this, &BattleScene::handleDropCommand);
}

void BattleScene::syncFromState() {
    clearHighlights();
    Board* board = activeBoard();

    for (UnitItem* item : unitItems) {
        if (!item || !item->unit()) {
            continue;
        }

        const int unitId = item->unit()->id();
        Character* character = dynamic_cast<Character*>(item->unit());
        if (character && character->getteam().getteam() == teams::enemy) {
            item->setVisible(true);
            item->setZValue(zUnit);
            Hex h;
            if (board && board->posOf(item->unit(), &h)) {
                item->setHex(h);
                item->setPos(layout->toWorld(h));
            }
            item->update();
            consumeVisualEvents(item, character);
            continue;
        }

        if (isBattleDisplayUnit(item->unit())) {
            item->setVisible(battlePhase == BattlePhase::Battle);
            item->setZValue(zUnit);

            Hex h;
            if (!board || !board->posOf(item->unit(), &h)) {
                item->setVisible(false);
                continue;
            }

            item->setHex(h);
            item->setPos(layout->toWorld(h));
            item->update();
            consumeVisualEvents(item, character);
            continue;
        }

        if (battlePhase == BattlePhase::Battle) {
            item->setVisible(false);
            continue;
        }

        const auto placementIt = places.find(unitId);
        if (placementIt == places.end() || placementIt->second.area == UnitArea::Hidden) {
            item->setVisible(false);
            continue;
        }

        const Placement& placement = placementIt->second;
        item->setVisible(true);
        item->setZValue(zUnit);

        if (placement.area == UnitArea::Board) {
            if (!placement.hasHex ||
                !board ||
                !board->has(placement.hex) ||
                board->unitAt(placement.hex) != item->unit()) {
                item->setVisible(false);
                continue;
            }

            item->setHex(placement.hex);
            item->setPos(layout->toWorld(placement.hex));
        } else if (placement.area == UnitArea::Bench) {
            item->clearHex();
            item->setZValue(zBenchUnit);
            item->setPos(benchSlotCenter(placement.slot));
        }
        item->update();
    }
    updateDeploymentText();
}

bool BattleScene::isBenchSlotFree(const QPoint& slotPos, int ignoredUnitId) const {
    for (const auto& entry : places) {
        if (entry.first == ignoredUnitId) {
            continue;
        }
        if (entry.second.area == UnitArea::Bench && entry.second.slot == slotPos) {
            return false;
        }
    }
    return true;
}

QPoint BattleScene::firstFreeBenchSlot() const {
    for (int row = 0; row < benchRows; ++row) {
        for (int col = 0; col < benchCols; ++col) {
            const QPoint slot(col, row);
            if (isBenchSlotFree(slot)) {
                return slot;
            }
        }
    }
    return QPoint(-1, -1);
}

BattleScene::DropTarget BattleScene::dropTargetAt(const QPointF& scenePos) const {
    if (BenchSlotItem* benchSlot = findBenchSlotItemAt(scenePos)) {
        DropTarget target;
        target.area = UnitArea::Bench;
        target.slot = benchSlot->slotPos();
        target.valid = true;
        return target;
    }

    const auto target = layout->fromWorld(scenePos);
    GridItem* targetItem = target ? findGridItem(*target) : nullptr;
    if (!targetItem) {
        return {};
    }

    const QPointF localPos = targetItem->mapFromScene(scenePos);
    if (!targetItem->boundingRect().contains(localPos)) {
        return {};
    }

    DropTarget out;
    out.area = UnitArea::Board;
    out.hex = *target;
    out.hasHex = true;
    out.valid = true;
    return out;
}

QPointF BattleScene::benchSlotTopLeft(int row, int col) const {
    return QPointF(benchOrigin.x() + col * (benchSize + benchGap),
                   benchOrigin.y() + row * (benchSize + benchGap));
}

QPointF BattleScene::benchSlotCenter(const QPoint& slotPos) const {
    return benchSlotTopLeft(slotPos.y(), slotPos.x()) +
           QPointF(benchSize * 0.5, benchSize * 0.5);
}



