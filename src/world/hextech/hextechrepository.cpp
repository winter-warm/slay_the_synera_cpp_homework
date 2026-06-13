#include "hextechrepository.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>
#include <random>

static QJsonObject loadRoot() {
    const QString runtimePath = QCoreApplication::applicationDirPath() + "/hextech/hextech.json";
    const QString sourcePath = QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath("hextech.json");

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

static HexTechEffect effectFromJson(const QJsonObject& object) {
    HexTechEffect effect;
    effect.type = object.value("type").toString().toStdString();
    effect.value = object.value("value").toInt();
    effect.buffId = object.value("buffId").toInt();
    effect.duration = object.value("duration").toInt();
    effect.modifierKey = object.value("modifierKey").toString().toStdString();
    return effect;
}

static HexTechDefinition definitionFromJson(const QJsonObject& object) {
    HexTechDefinition definition;
    definition.id = object.value("id").toString().toStdString();
    definition.layerId = object.value("layerId").toInt(1);
    definition.name = object.value("name").toString().toStdString();
    definition.title = object.value("title").toString().toStdString();
    definition.description = object.value("description").toString().toStdString();
    definition.cardImagePath = object.value("cardImagePath").toString().toStdString();

    const QJsonArray effects = object.value("effects").toArray();
    for (const QJsonValue& value : effects) {
        definition.effects.push_back(effectFromJson(value.toObject()));
    }
    return definition;
}

HexTechRepository::HexTechRepository()
    : root(loadRoot()) {}

std::vector<HexTechDefinition> HexTechRepository::allForLayer(int layerId) const {
    std::vector<HexTechDefinition> definitions;
    const QJsonArray cards = root.value("cards").toArray();
    for (const QJsonValue& value : cards) {
        HexTechDefinition definition = definitionFromJson(value.toObject());
        if (definition.layerId == layerId) {
            definitions.push_back(std::move(definition));
        }
    }
    return definitions;
}

std::vector<HexTechDefinition> HexTechRepository::choicesFor(int seed, int layerId, int nodeId, int count) const {
    std::vector<HexTechDefinition> pool = allForLayer(layerId);
    std::mt19937 rng(static_cast<unsigned int>(seed * 1103515245u +
                                               layerId * 4099u +
                                               nodeId * 9176u));
    std::shuffle(pool.begin(), pool.end(), rng);
    if (static_cast<int>(pool.size()) > count) {
        pool.resize(static_cast<size_t>(count));
    }
    return pool;
}

std::optional<HexTechDefinition> HexTechRepository::findById(const std::string& id) const {
    const QJsonArray cards = root.value("cards").toArray();
    for (const QJsonValue& value : cards) {
        HexTechDefinition definition = definitionFromJson(value.toObject());
        if (definition.id == id) {
            return definition;
        }
    }
    return std::nullopt;
}
