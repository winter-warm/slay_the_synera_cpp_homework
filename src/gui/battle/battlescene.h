#ifndef GUI_BATTLE_BATTLESCENE_H
#define GUI_BATTLE_BATTLESCENE_H

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
class Object;
class Obstacle;
class PropItem;
class QGraphicsRectItem;
class QGraphicsItem;
class QGraphicsScene;
class QGraphicsTextItem;
class Unit;
class UnitItem;

class BattleScene : public QObject {
    Q_OBJECT

public:
    struct EquipmentRewardVisual {
        EquipmentGroup group = EquipmentGroup::Mold;
        int equipmentId = 0;
        std::string iconPath;
    };

    explicit BattleScene(QObject* parent = nullptr);
    ~BattleScene();

    void initialize();
    void setBoard(Board* board);
    void resetPreparation();
    void clearRoster();
    void setDeploymentLimit(int limit);
    void setBattleCharacters(const std::vector<Character*>& characters);
    void removeCharacterItem(int id);
    void syncFromBattleSystem(const battlesystem& battleSystem);
    bool spawnRewardChest(int seedSalt);
    void playRewardEffects(int gold, const std::vector<EquipmentRewardVisual>& equipmentDrops);
    std::vector<CharacterPlacement> preparedPlacements() const;
    bool preparationActive() const;
    int pcRosterUnitIdAtScenePos(const QPointF& scenePos) const;

    Unit* addCharacter(Character* character, const std::string& displayName = "Character");

    QGraphicsScene* scene() const { return sceneObj; }

    void handleDragStarted(int unitId, const QPointF& scenePos);
    void handleDragMoved(int unitId, const QPointF& scenePos);
    void handleDropCommand(int unitId, const QPointF& scenePos);

    int phase = 0;
signals:
    void rewardChestOpened();
    void rewardEquipmentClaimed(EquipmentGroup group, int equipmentId);
    void placementsChanged();
    void rosterUnitRecycleRequested(int unitId);

private:
    enum class BattlePhase { Prep, Battle };
    enum class UnitArea { Hidden, Board, Bench, Trash };

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
    bool isTrashAt(const QPointF& scenePos) const;
    void setTrashHover(bool hover, bool accepted);
    bool canApplyDrop(int unitId, const DropTarget& target) const;
    void applyDrop(int unitId, const DropTarget& target);
    int boardRosterCount() const;
    Unit* boardRosterUnitWithName(const std::string& name, int ignoredUnitId = -1) const;
    void moveUnitToBench(Unit* unit);
    void updateDeploymentText();
    void consumeVisualEvents(UnitItem* item, Character* character);
    void playHitFlash(UnitItem* item);
    void playAttackLunge(UnitItem* item, UnitItem* targetItem);
    void playSkillBurst(UnitItem* item);
    void clearRewardChest();
    void createObjectPropItem(Object* object);
    void createObstaclePropItem(Obstacle* obstacle);
    void removeObjectItem(int id);
    void handleObjectOpened(int objectId);
    void playChestRewardBurst(const QPointF& center);
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
    QGraphicsTextItem* deploymentTextItem;
    QGraphicsRectItem* trashItem;
    QGraphicsTextItem* trashTextItem;
    std::vector<BenchSlotItem*> benchItems;
    std::vector<UnitItem*> unitItems;
    std::vector<QGraphicsItem*> obstacleItems;
    std::unique_ptr<Object> rewardChest;
    PropItem* rewardChestItem;

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
    int deploymentLimit;
};

#endif // GUI_BATTLE_BATTLESCENE_H


