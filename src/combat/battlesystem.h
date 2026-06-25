#ifndef BATTLESYSTEM_H
#define BATTLESYSTEM_H

#include "core/board.h"
#include "core/hex.h"
#include "combat/equipment/equipment.h"
#include "entity/component/teamcomponent.h"
#include "world/aura/aura.h"
#include "world/event/eventtypes.h"
#include <QObject>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Character;
enum class Bond:int;

struct CharacterPlacement {
    Character* source = nullptr;
    Hex hex;
    bool hasEquipment = false;
    EquipmentGroup equipmentGroup = EquipmentGroup::Mold;
    int equipmentId = 0;
};

struct CharacterRecord {
    std::unique_ptr<Character> character;
    Hex position;
    teams team = teams::pc;
};

class battlesystem : public QObject
{
    Q_OBJECT

public:
    explicit battlesystem(QObject* parent = nullptr);
    ~battlesystem() override;

    bool loadBattle(const BattleConfig& config);
    void setActiveAuraIds(const std::vector<std::string>& auraIds);
    static std::unordered_map<Bond, int> count_bond(const std::vector<CharacterPlacement>& placements);
    bool startFromPreparedUnits(const std::vector<CharacterPlacement>& placements);
    void update(float deltaTime);
    void clear();

    Board& mutableBoard() { return board; }
    const Board& getBoard() const { return board; }
    std::vector<Character*> liveCharacters() const;
    bool isRunning() const { return running; }

signals:
    void battleStarted();
    void characterRemoved(int characterId);
    void boardChanged();
    void battleEnded(bool pcTeamAllDead);

private:
    std::string boardPathForId(const std::string& boardId) const;
    void applyStatMultiplier(Character* character, float multiplier);
    void addAuraBuffs(Character* character);
    void addEquipmentBuffs(Character* character, const Character* source);
    void addEquipmentBuffs(Character* character, EquipmentGroup group, int equipmentId);
    void applyBondEffects(const std::unordered_map<Bond, int>& bondCounts);
    void addRecordFromSource(Character* source, const Hex& hex, bool hasEquipment = false,
                             EquipmentGroup equipmentGroup = EquipmentGroup::Mold, int equipmentId = 0);
    void removeDeadCharacters();
    void finishIfTeamDead();

    Board board;
    std::unordered_map<int, CharacterRecord> characters;
    std::unordered_set<int> pcTeamIds;
    std::unordered_set<int> enemyTeamIds;
    std::vector<std::unique_ptr<Character>> stagedEnemies;
    std::vector<ActiveAura> activeAuras;
    BattleKind currentBattleKind = BattleKind::None;
    bool running = false;
};

#endif // BATTLESYSTEM_H
