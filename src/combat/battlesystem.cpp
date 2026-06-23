#include "battlesystem.h"
#include "combat/buff/bufffactory.h"
#include "entity/character/character.h"
#include "entity/character/characterfactory.h"
#include "entity/equipskill/equipskill.h"
#include "world/hextech/hextechrepository.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <algorithm>
#include <cmath>

battlesystem::battlesystem(QObject* parent)
    : QObject(parent) {}

battlesystem::~battlesystem() = default;

bool battlesystem::loadBattle(const BattleConfig& config) {
    clear();
    currentBattleKind = config.kind;

    if (!board.load(boardPathForId(config.boardId))) {
        if (!board.load(boardPathForId("default_board"))) {
            emit boardChanged();
            return false;
        }
    }

    for (const EnemyPlacement& placement : config.enemies) {
        if (!board.has(placement.hex) || !board.passable(placement.hex)) {
            qDebug() << "Skipping enemy with invalid or blocked battle position"
                     << placement.hex.x << placement.hex.y << placement.hex.z;
            continue;
        }

        std::unique_ptr<Character> enemy = characterfactory::create(placement.templateId);
        if (!enemy) {
            continue;
        }
        applyStatMultiplier(enemy.get(), config.statMultiplier);

        Character* raw = enemy.get();
        if (board.add(raw, placement.hex)) {
            stagedEnemies.push_back(std::move(enemy));
        } else {
            qDebug() << "Failed to place enemy" << placement.templateId
                     << "at" << placement.hex.x << placement.hex.y << placement.hex.z;
        }
    }

    emit boardChanged();
    return true;
}

void battlesystem::setActiveAuraIds(const std::vector<std::string>& auraIds) {
    activeAuras.clear();
    HexTechRepository repository;

    for (const std::string& auraId : auraIds) {
        const auto definition = repository.findById(auraId);
        if (!definition) {
            continue;
        }

        ActiveAura aura;
        aura.hexTechId = definition->id;
        for (const HexTechEffect& effect : definition->effects) {
            if (isLongTermHexTechEffect(effect)) {
                aura.effects.push_back(effect);
            }
        }
        if (!aura.effects.empty()) {
            activeAuras.push_back(std::move(aura));
        }
    }
}

bool battlesystem::startFromPreparedUnits(const std::vector<CharacterPlacement>& placements) {
    if (running) {
        return false;
    }

    characters.clear();
    pcTeamIds.clear();
    enemyTeamIds.clear();

    for (const CharacterPlacement& placement : placements) {
        if (placement.source) {
            addRecordFromSource(placement.source, placement.hex);
        }
    }

    std::vector<std::pair<Character*, Hex>> enemies;
    for (const auto& enemy : stagedEnemies) {
        if (!enemy) {
            continue;
        }
        Hex hex;
        if (board.posOf(enemy.get(), &hex)) {
            enemies.push_back({enemy.get(), hex});
        }
    }
    for (const auto& enemy : enemies) {
        addRecordFromSource(enemy.first, enemy.second);
    }

    if (pcTeamIds.empty() || enemyTeamIds.empty()) {
        clear();
        return false;
    }

    for (const auto& placement : placements) {
        if (placement.source) {
            board.remove(placement.source);
        }
    }
    for (const auto& enemy : stagedEnemies) {
        if (enemy) {
            board.remove(enemy.get());
        }
    }

    for (auto& entry : characters) {
        Character* character = entry.second.character.get();
        if (board.add(character, entry.second.position)) {
            character->setCurrentBoard(&board);
            character->onBattleStart();
        }
    }

    running = true;
    stagedEnemies.clear();
    emit battleStarted();
    emit boardChanged();
    return true;
}

void battlesystem::update(float deltaTime) {
    if (!running) {
        return;
    }

    std::vector<Character*> list;
    list.reserve(characters.size());
    for (auto& entry : characters) {
        if (entry.second.character) {
            list.push_back(entry.second.character.get());
        }
    }

    for (Character* character : list) {
        if (character && characters.find(character->id()) != characters.end()) {
            character->setCurrentBoard(&board);
            character->update(deltaTime, board);
        }
    }

    removeDeadCharacters();
    finishIfTeamDead();
    emit boardChanged();
}

void battlesystem::clear() {
    for (auto& entry : characters) {
        if (entry.second.character) {
            board.remove(entry.second.character.get());
        }
    }
    for (const auto& enemy : stagedEnemies) {
        if (enemy) {
            board.remove(enemy.get());
        }
    }

    characters.clear();
    pcTeamIds.clear();
    enemyTeamIds.clear();
    stagedEnemies.clear();
    running = false;
    board.clearUnits();
    emit boardChanged();
}

std::vector<Character*> battlesystem::liveCharacters() const {
    std::vector<Character*> out;
    out.reserve(characters.size());
    for (const auto& entry : characters) {
        if (entry.second.character) {
            out.push_back(entry.second.character.get());
        }
    }
    return out;
}

std::string battlesystem::boardPathForId(const std::string& boardId) const {
    const QString fileName = QString::fromStdString(boardId) + ".json";
    const QString appPath = QCoreApplication::applicationDirPath() + "/boards/" + fileName;
    if (QFile::exists(appPath)) {
        return appPath.toStdString();
    }

    const QString localPath = QDir("src/core/boards").filePath(fileName);
    if (QFile::exists(localPath)) {
        return localPath.toStdString();
    }

    return (QCoreApplication::applicationDirPath() + "/boards/default_board.json").toStdString();
}

void battlesystem::applyStatMultiplier(Character* character, float multiplier) {
    if (!character || multiplier <= 0.0f) {
        return;
    }

    StatsComponent& stats = character->getstats();
    const int maxHp = std::max(1, static_cast<int>(std::lround(stats.getMAXHP() * multiplier)));
    const int attack = std::max(0, static_cast<int>(std::lround(stats.getATTACK() * multiplier)));
    const int defense = std::max(0, static_cast<int>(std::lround(stats.getDEFENSE() * multiplier)));

    stats.modifyMAXHP(maxHp - stats.getMAXHP());
    stats.setHP(maxHp);
    stats.setATTACK(attack);
    stats.modifyDEFESE(defense - stats.getDEFENSE());
}

void battlesystem::addAuraBuffs(Character* character) {
    if (!character) {
        return;
    }

    for (const ActiveAura& aura : activeAuras) {
        for (const HexTechEffect& effect : aura.effects) {
            if (effect.type != "apply_buff" || effect.buffId <= 0) {
                continue;
            }
            if (effect.modifierKey == "boss_only" && currentBattleKind != BattleKind::Boss) {
                continue;
            }
            if (effect.modifierKey == "boss_elite_only" &&
                currentBattleKind != BattleKind::Boss &&
                currentBattleKind != BattleKind::Elite) {
                continue;
            }

            std::unique_ptr<buff> created = bufffactory::create(effect.buffId, effect.duration);
            if (created) {
                character->getbuff().addBuff(std::move(created));
            }
        }
    }
}

void battlesystem::addEquipmentBuffs(Character* character, const Character* source) {
    if (!character || !source || !source->getquipe()) {
        return;
    }

    equipskill::addEquipmentStartBuffs(character, *source->getquipe());
    equipskill::addEquipmentActiveSkills(character, *source->getquipe());
}

void battlesystem::addRecordFromSource(Character* source, const Hex& hex) {
    if (!source) {
        return;
    }

    auto copy = std::make_unique<Character>(*source);
    Character* character = copy.get();
    character->clearPosition();
    addEquipmentBuffs(character, source);
    character->getattack().activateAndRemoveBuffSkills();
    addAuraBuffs(character);

    CharacterRecord record;
    record.position = hex;
    record.team = character->getteam().getteam();
    record.character = std::move(copy);

    const int id = character->id();
    if (record.team == teams::enemy) {
        enemyTeamIds.insert(id);
    } else {
        pcTeamIds.insert(id);
    }
    characters.emplace(id, std::move(record));
}

void battlesystem::removeDeadCharacters() {
    std::vector<int> deadIds;
    for (auto& entry : characters) {
        Character* character = entry.second.character.get();
        if (character && character->isdead()) {
            deadIds.push_back(entry.first);
        }
    }

    for (int id : deadIds) {
        auto it = characters.find(id);
        if (it == characters.end()) {
            continue;
        }

        if (it->second.character) {
            board.remove(it->second.character.get());
        }
        pcTeamIds.erase(id);
        enemyTeamIds.erase(id);
        characters.erase(it);
        emit characterRemoved(id);
    }
}

void battlesystem::finishIfTeamDead() {
    if (!running) {
        return;
    }
    if (!pcTeamIds.empty() && !enemyTeamIds.empty()) {
        return;
    }

    running = false;
    const bool pcAllDead = pcTeamIds.empty();
    emit battleEnded(pcAllDead);
}
