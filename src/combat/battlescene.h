#ifndef COMBAT_BATTLESCENE_H
#define COMBAT_BATTLESCENE_H

#include <QObject>
#include <QPoint>
#include <QPointF>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "combat/battlesystem.h"
#include "core/board.h"

class BenchSlotItem;
class Character;
class GridItem;
class HexLayout;
class QGraphicsScene;
class Unit;
class UnitItem;

class BattleScene : public QObject {
    Q_OBJECT

public:
    explicit BattleScene(QObject* parent = nullptr);
    ~BattleScene();

    void initialize();
    void setBoard(Board* board);
    void resetPreparation();
    void clearRoster();
    void setBattleCharacters(const std::vector<Character*>& characters);
    void removeCharacterItem(int id);
    void syncFromBattleSystem(const battlesystem& battleSystem);
    std::vector<CharacterPlacement> preparedPlacements() const;

    Unit* addCharacter(Character* character, const std::string& displayName = "Character");

    QGraphicsScene* scene() const { return sceneObj; }

    void handleDragStarted(int unitId, const QPointF& scenePos);
    void handleDragMoved(int unitId, const QPointF& scenePos);
    void handleDropCommand(int unitId, const QPointF& scenePos);

    int phase = 0;
private:
    enum class BattlePhase { Prep, Battle };
    enum class UnitArea { Hidden, Board, Bench };

    struct Placement {
        UnitArea area = UnitArea::Hidden;
        Hex hex;
        bool hasHex = false;
        QPoint slot = QPoint(-1, -1);
    };

    struct DropTarget {
        UnitArea area = UnitArea::Hidden;
        Hex hex;
        bool hasHex = false;
        QPoint slot = QPoint(-1, -1);
        bool valid = false;
    };

    Unit* findUnitById(int unitId) const;
    bool isRosterUnit(Unit* unit) const;
    bool isBattleDisplayUnit(Unit* unit) const;
    bool isRenderedBattleId(int unitId) const;
    void clearTransientUnitItems();
    void hideBench(bool hidden);
    Board* activeBoard() const;
    QRectF rawBoardBounds() const;
    bool isPreparationPhase() const;
    bool canDragUnit(int unitId) const;
    GridItem* findGridItem(const Hex& hex) const;
    BenchSlotItem* findBenchSlotItem(const QPoint& slotPos) const;
    BenchSlotItem* findBenchSlotItemAt(const QPointF& scenePos) const;
    UnitItem* findUnitItem(int unitId) const;
    void clearHighlights();
    bool canApplyDrop(int unitId, const DropTarget& target) const;
    void applyDrop(int unitId, const DropTarget& target);
    void buildScene();
    void createUnitItem(Unit* unit);
    void syncFromState();
    bool isBenchSlotFree(const QPoint& slotPos, int ignoredUnitId = -1) const;
    QPoint firstFreeBenchSlot() const;
    DropTarget dropTargetAt(const QPointF& scenePos) const;

    QPointF benchSlotTopLeft(int row, int col) const;
    QPointF benchSlotCenter(const QPoint& slotPos) const;

    Board* boardRef;
    std::unique_ptr<HexLayout> layout;
    std::vector<Unit*> units;
    std::vector<Character*> battleDisplayUnits;

    QGraphicsScene* sceneObj;
    std::vector<GridItem*> gridItems;
    std::vector<BenchSlotItem*> benchItems;
    std::vector<UnitItem*> unitItems;

    bool dragActive;
    int activeUnitId;
    std::unordered_map<int, UnitItem*> unitItemMap;
    std::unordered_map<int, Placement> places;

    int benchRows;
    int benchCols;
    qreal benchSize;
    qreal benchGap;
    QPointF benchOrigin;
    BattlePhase battlePhase;
};

#endif // COMBAT_BATTLESCENE_H


