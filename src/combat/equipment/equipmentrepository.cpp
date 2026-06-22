#include "equipmentrepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

static QJsonObject loadEquipmentRoot(const QString& fileName) {
    const QString runtimePath = QCoreApplication::applicationDirPath() + "/equipment/" + fileName;
    const QString sourcePath = QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath(fileName);

    for (const QString& path : {runtimePath, sourcePath}) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            return doc.object();
        }
    }
    return {};
}

static EquipmentGroup groupFromInt(int value) {
    if (value == static_cast<int>(EquipmentGroup::Element)) {
        return EquipmentGroup::Element;
    }
    if (value == static_cast<int>(EquipmentGroup::Advanced)) {
        return EquipmentGroup::Advanced;
    }
    return EquipmentGroup::Mold;
}

static std::optional<EquipmentStat> statFromString(const QString& value) {
    if (value == "attack_percent") {
        return EquipmentStat::AttackPercent;
    }
    if (value == "attack_speed_percent") {
        return EquipmentStat::AttackSpeedPercent;
    }
    if (value == "attack_range_flat") {
        return EquipmentStat::AttackRangeFlat;
    }
    if (value == "crit_chance_percent") {
        return EquipmentStat::CritChancePercent;
    }
    if (value == "crit_damage_percent") {
        return EquipmentStat::CritDamagePercent;
    }
    if (value == "initial_mp_flat") {
        return EquipmentStat::InitialMpFlat;
    }
    if (value == "max_mp_flat") {
        return EquipmentStat::MaxMpFlat;
    }
    if (value == "max_hp_percent") {
        return EquipmentStat::MaxHpPercent;
    }
    if (value == "initial_shield_max_hp_percent") {
        return EquipmentStat::InitialShieldMaxHpPercent;
    }
    if (value == "defense_percent") {
        return EquipmentStat::DefensePercent;
    }
    if (value == "dodge_chance_percent") {
        return EquipmentStat::DodgeChancePercent;
    }
    if (value == "healing_done_percent") {
        return EquipmentStat::HealingDonePercent;
    }
    if (value == "healing_taken_percent") {
        return EquipmentStat::HealingTakenPercent;
    }
    return std::nullopt;
}

static std::vector<EquipmentModifier> modifiersFromJson(const QJsonArray& array) {
    std::vector<EquipmentModifier> modifiers;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        const auto stat = statFromString(object.value("stat").toString());
        if (!stat) {
            continue;
        }
        modifiers.push_back({*stat, object.value("value").toInt()});
    }
    return modifiers;
}

static std::vector<int> buffIdsFromJson(const QJsonArray& array) {
    std::vector<int> buffIds;
    for (const QJsonValue& value : array) {
        const int buffId = value.toInt();
        if (buffId > 0) {
            buffIds.push_back(buffId);
        }
    }
    return buffIds;
}

static std::optional<EquipmentRecipe> recipeFromJson(const QJsonObject& object) {
    if (object.isEmpty()) {
        return std::nullopt;
    }

    EquipmentRecipe recipe;
    recipe.moldId = object.value("moldId").toInt();
    recipe.elementId = object.value("elementId").toInt();
    if (recipe.moldId <= 0 || recipe.elementId <= 0) {
        return std::nullopt;
    }
    return recipe;
}

static Equipment equipmentFromJson(const QJsonObject& object) {
    return Equipment(object.value("id").toInt(),
                     groupFromInt(object.value("group").toInt()),
                     object.value("name").toString().toStdString(),
                     object.value("title").toString().toStdString(),
                     object.value("description").toString().toStdString(),
                     object.value("iconPath").toString().toStdString(),
                     object.value("effectText").toString().toStdString(),
                     modifiersFromJson(object.value("modifiers").toArray()),
                     buffIdsFromJson(object.value("buffIds").toArray()),
                     object.value("skillName").toString().toStdString(),
                     object.value("skillText").toString().toStdString(),
                     recipeFromJson(object.value("recipe").toObject()));
}

static std::vector<Equipment> equipmentListFromRoot(const QJsonObject& root) {
    std::vector<Equipment> equipment;
    const QJsonArray array = root.value("equipment").toArray();
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        if (object.value("id").toInt() > 0) {
            equipment.push_back(equipmentFromJson(object));
        }
    }
    return equipment;
}

EquipmentRepository::EquipmentRepository()
    : basicEquipment(equipmentListFromRoot(loadEquipmentRoot("basic_equipment.json"))),
      advancedEquipment(equipmentListFromRoot(loadEquipmentRoot("advanced_equipment.json"))) {}

const std::vector<Equipment>& EquipmentRepository::allBasic() const {
    return basicEquipment;
}

const std::vector<Equipment>& EquipmentRepository::allAdvanced() const {
    return advancedEquipment;
}

std::optional<Equipment> EquipmentRepository::findBasic(EquipmentGroup group, int id) const {
    for (const Equipment& equipment : basicEquipment) {
        if (equipment.group() == group && equipment.id() == id) {
            return equipment;
        }
    }
    return std::nullopt;
}

std::optional<Equipment> EquipmentRepository::findAdvanced(int id) const {
    for (const Equipment& equipment : advancedEquipment) {
        if (equipment.id() == id) {
            return equipment;
        }
    }
    return std::nullopt;
}

bool EquipmentRepository::canCompose(const Equipment& left, const Equipment& right) const {
    if (!left.isBasic() || !right.isBasic()) {
        return false;
    }
    if (left.group() == right.group()) {
        return false;
    }
    return composeResult(left, right).has_value();
}

std::optional<Equipment> EquipmentRepository::composeResult(const Equipment& left, const Equipment& right) const {
    const Equipment* mold = nullptr;
    const Equipment* element = nullptr;
    if (left.group() == EquipmentGroup::Mold && right.group() == EquipmentGroup::Element) {
        mold = &left;
        element = &right;
    } else if (left.group() == EquipmentGroup::Element && right.group() == EquipmentGroup::Mold) {
        mold = &right;
        element = &left;
    } else {
        return std::nullopt;
    }

    return findAdvanced(mold->id() * 10 + element->id());
}
