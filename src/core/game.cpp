#include "game.h"
#include "entity/character/character.h"
#include "entity/unit.h"
#include "gui/benchslotitem.h"
#include "gui/griditem.h"
#include "gui/hexlayout.h"
#include "gui/unititem.h"
#include <QGraphicsScene>


static constexpr qreal zGrid = 0.0, zBench = 0.2, zUnit = 1.0, zDraggingUnit = 2.0;

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

Game::Game(QObject* parent)
    : QObject(parent)
    , layout(std::make_unique<HexLayout>())
    , sceneObj(new QGraphicsScene(this))
    , dragActive(false)
    , activeUnitId(-1)
    , benchRows(2)
    , benchCols(12)
    , benchSize(78.0)
    , benchGap(10.0)
    , benchOrigin(0.0, 0.0) {}

Game::~Game() {
    units.clear();
}

void Game::initialize() {
    board.load("maps/default_board.json");
    buildScene();
    reset();
}

void Game::reset() {
    board.clearUnits();
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
    syncFromState();
}

Unit* Game::addCharacter(Character* character, const std::string& displayName) {
    if (!character) {
        return nullptr;
    }

    Unit* unit = character;
    unit->setName(displayName);
    units.append(unit);

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

void Game::handleDragStarted(int unitId, const QPointF&) {
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

void Game::handleDragMoved(int unitId, const QPointF& scenePos) {
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

void Game::handleDropCommand(int unitId, const QPointF& scenePos) {
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

Unit* Game::findUnitById(int unitId) const {
    for (Unit* unit : units) {
        if (unit && unit->id() == unitId) {
            return unit;
        }
    }
    return nullptr;
}

bool Game::isPreparationPhase() const {
    // TODO: Replace with battle phase timing once it exists.
    return true;
}

bool Game::canDragUnit(int unitId) const {
    Unit* unit = findUnitById(unitId);
    return isPreparationPhase() && dynamic_cast<Character*>(unit);
}

GridItem* Game::findGridItem(const Hex& hex) const {
    for (GridItem* item : gridItems) {
        if (item && item->hex() == hex) {
            return item;
        }
    }
    return nullptr;
}

BenchSlotItem* Game::findBenchSlotItem(const QPoint& slotPos) const {
    for (BenchSlotItem* item : benchItems) {
        if (item && item->slotPos() == slotPos) {
            return item;
        }
    }
    return nullptr;
}

BenchSlotItem* Game::findBenchSlotItemAt(const QPointF& scenePos) const {
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

UnitItem* Game::findUnitItem(int unitId) const {
    auto it = unitItemMap.find(unitId);
    if (it == unitItemMap.end()) {
        return nullptr;
    }
    return it->second;
}

void Game::clearHighlights() {
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

bool Game::canApplyDrop(int unitId, const DropTarget& target) const {
    Unit* unit = findUnitById(unitId);
    if (!unit || !canDragUnit(unitId) || !target.valid) {
        return false;
    }

    const auto placementIt = places.find(unitId);
    if (placementIt == places.end() || placementIt->second.area == UnitArea::Hidden) {
        return false;
    }

    const Placement& source = placementIt->second;

    if (target.area == UnitArea::Board) {
        const Board::Zone z = target.hasHex ? board.zone(target.hex) : Board::Zone::None;
        if (!target.hasHex ||
            !board.has(target.hex) ||
            !board.empty(target.hex) ||
            (z != Board::Zone::Player && z != Board::Zone::Neutral)) {
            return false;
        }
        if (source.area == UnitArea::Board) {
            return source.hasHex && board.unitAt(source.hex) == unit && source.hex != target.hex;
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

void Game::applyDrop(int unitId, const DropTarget& target) {
    Unit* unit = findUnitById(unitId);
    if (!unit) {
        return;
    }

    const auto placementIt = places.find(unitId);
    if (placementIt != places.end() && placementIt->second.area == UnitArea::Board) {
        board.remove(unit);
    }

    if (target.area == UnitArea::Board) {
        if (target.hasHex) {
            board.add(unit, target.hex);
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
}

void Game::buildScene() {
    sceneObj->clear();
    gridItems.clear();
    benchItems.clear();
    unitItems.clear();
    unitItemMap.clear();
    layout->setCells(board.cells());

    QRectF totalBounds;
    bool first = true;
    for (const Hex& hex : board.cells()) {
        const QPolygonF poly = layout->poly(hex);
        GridItem* gridItem = new GridItem(hex, poly);
        gridItem->setZValue(zGrid);
        gridItem->setBaseColor(colorForZone(board.zone(hex)));

        sceneObj->addItem(gridItem);
        gridItems.push_back(gridItem);

        const QRectF bounds = gridItem->sceneBoundingRect();
        totalBounds = first ? bounds : totalBounds.united(bounds);
        first = false;
    }

    benchOrigin = QPointF(totalBounds.left(), totalBounds.bottom() + 64.0);
    const QRectF slotRect(0.0, 0.0, benchSize, benchSize);

    for (int row = 0; row < benchRows; ++row) {
        for (int col = 0; col < benchCols; ++col) {
            BenchSlotItem* slotItem = new BenchSlotItem(row, col, slotRect);
            slotItem->setZValue(zBench);
            slotItem->setPos(benchSlotTopLeft(row, col));
            sceneObj->addItem(slotItem);
            benchItems.push_back(slotItem);
            totalBounds = totalBounds.united(slotItem->sceneBoundingRect());
        }
    }

    for (Unit* unit : units) {
        createUnitItem(unit);
    }

    sceneObj->setSceneRect(totalBounds.adjusted(-40, -40, 40, 40));
}

void Game::createUnitItem(Unit* unit) {
    if (!unit || unitItemMap.find(unit->id()) != unitItemMap.end()) {
        return;
    }

    UnitItem* unitItem = new UnitItem(unit);
    unitItem->setZValue(zUnit);
    sceneObj->addItem(unitItem);
    unitItems.push_back(unitItem);
    unitItemMap[unit->id()] = unitItem;

    connect(unitItem, &UnitItem::dragStarted,
            this, &Game::handleDragStarted);
    connect(unitItem, &UnitItem::dragMoved,
            this, &Game::handleDragMoved);
    connect(unitItem, &UnitItem::dragDropped,
            this, &Game::handleDropCommand);
}

void Game::syncFromState() {
    clearHighlights();

    for (UnitItem* item : unitItems) {
        if (!item || !item->unit()) {
            continue;
        }

        const int unitId = item->unit()->id();
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
                !board.has(placement.hex) ||
                board.unitAt(placement.hex) != item->unit()) {
                item->setVisible(false);
                continue;
            }

            item->setHex(placement.hex);
            item->setPos(layout->toWorld(placement.hex));
        } else if (placement.area == UnitArea::Bench) {
            item->clearHex();
            item->setPos(benchSlotCenter(placement.slot));
        }
    }
}

bool Game::isBenchSlotFree(const QPoint& slotPos, int ignoredUnitId) const {
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

QPoint Game::firstFreeBenchSlot() const {
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

Game::DropTarget Game::dropTargetAt(const QPointF& scenePos) const {
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

QPointF Game::benchSlotTopLeft(int row, int col) const {
    return QPointF(benchOrigin.x() + col * (benchSize + benchGap),
                   benchOrigin.y() + row * (benchSize + benchGap));
}

QPointF Game::benchSlotCenter(const QPoint& slotPos) const {
    return benchSlotTopLeft(slotPos.y(), slotPos.x()) +
           QPointF(benchSize * 0.5, benchSize * 0.5);
}
