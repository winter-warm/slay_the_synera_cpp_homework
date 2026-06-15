#include "battlerepository.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <algorithm>
#include <random>

static QJsonObject loadRoot() {
    const QString runtimePath = QCoreApplication::applicationDirPath() + "/battle/battle.json";
    const QString sourcePath = QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath("battle.json");

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
        qDebug() << "Failed to parse battle.json:" << error.errorString();
    }
    qDebug() << "Failed to load battle.json";
    return {};
}

static int poolLayerFor(const BattleNodeContext& context) {
    if (context.kind == BattleKind::Boss) {
        return context.layerId * 10;
    }
    return context.layerId;
}

static bool eliteFlagFor(BattleKind kind) {
    return kind == BattleKind::Elite || kind == BattleKind::Boss;
}

static BattleKind kindFromPoolLayer(int poolLayer, bool elite) {
    if (poolLayer == 10 || poolLayer == 20 || poolLayer == 30) {
        return BattleKind::Boss;
    }
    return elite ? BattleKind::Elite : BattleKind::Normal;
}

static BattleConfig battleFromJson(const QJsonObject& object, const BattleNodeContext& context) {
    BattleConfig config;
    const int poolLayer = object.value("layer").toInt(context.layerId);
    const bool elite = object.value("elite").toBool(false);
    config.kind = kindFromPoolLayer(poolLayer, elite);
    config.boardId = object.value("map_id").toString("default_board").toStdString();
    config.statMultiplier = 1.0f + std::max(0, context.nodeRow - 1) * 0.05f;

    const QJsonArray enemies = object.value("enemy").toArray();
    for (const QJsonValue& value : enemies) {
        const QJsonObject enemyObject = value.toObject();
        const QJsonObject position = enemyObject.value("position").toObject();

        EnemyPlacement enemy;
        enemy.templateId = enemyObject.value("enemy").toInt();
        enemy.hex.x = position.value("x").toInt();
        enemy.hex.y = position.value("y").toInt();
        enemy.hex.z = -enemy.hex.x - enemy.hex.y;
        if (enemy.templateId <= 0 || !isValidHex(enemy.hex)) {
            qDebug() << "Skipping invalid battle enemy entry";
            continue;
        }
        config.enemies.push_back(enemy);
    }

    return config;
}

BattleRepository::BattleRepository()
    : root(loadRoot()) {}

std::optional<BattleConfig> BattleRepository::battleFor(const BattleNodeContext& context) const {
    const QJsonArray battles = root.value("battles").toArray();
    if (battles.isEmpty()) {
        qDebug() << "battle.json has no battles";
        return std::nullopt;
    }

    const int wantedLayer = poolLayerFor(context);
    const bool wantedElite = eliteFlagFor(context.kind);
    std::vector<QJsonObject> pool;

    for (const QJsonValue& value : battles) {
        const QJsonObject object = value.toObject();
        if (object.value("layer").toInt() != wantedLayer) {
            continue;
        }
        if (object.value("elite").toBool(false) != wantedElite) {
            continue;
        }
        if (object.value("enemy").toArray().isEmpty()) {
            qDebug() << "Skipping battle entry with empty enemy list";
            continue;
        }
        pool.push_back(object);
    }

    if (pool.empty()) {
        qDebug() << "No battle pool for layer" << wantedLayer << "elite" << wantedElite;
        return std::nullopt;
    }

    std::mt19937 rng(static_cast<unsigned int>(context.seed * 1103515245u +
                                               context.layerId * 1009u +
                                               context.nodeId * 9176u +
                                               context.nodeRow * 131u +
                                               static_cast<int>(context.kind) * 313u));
    std::uniform_int_distribution<int> dist(0, static_cast<int>(pool.size()) - 1);
    BattleConfig config = battleFromJson(pool[static_cast<size_t>(dist(rng))], context);
    if (config.enemies.empty()) {
        qDebug() << "Selected battle has no valid enemies";
        return std::nullopt;
    }
    return config;
}
