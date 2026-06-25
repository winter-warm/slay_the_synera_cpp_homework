#include "recordmanager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <utility>

static QJsonArray namesToJson(const std::vector<std::string>& names)
{
    QJsonArray array;
    for (const std::string& name : names) {
        array.append(QString::fromStdString(name));
    }
    return array;
}

static std::vector<std::string> namesFromJson(const QJsonArray& array)
{
    std::vector<std::string> names;
    for (const QJsonValue& value : array) {
        names.push_back(value.toString().toStdString());
    }
    return names;
}

static QJsonObject recordToJson(const RunRecord& record)
{
    QJsonObject object;
    object["result"] = QString::fromStdString(record.result);
    object["reachedLayer"] = record.reachedLayer;
    object["characterNames"] = namesToJson(record.characterNames);
    object["elapsedSeconds"] = record.elapsedSeconds;
    object["finishedAt"] = QString::fromStdString(record.finishedAt);
    return object;
}

static RunRecord recordFromJson(const QJsonObject& object)
{
    RunRecord record;
    record.result = object.value("result").toString().toStdString();
    record.reachedLayer = object.value("reachedLayer").toInt(1);
    record.characterNames = namesFromJson(object.value("characterNames").toArray());
    record.elapsedSeconds = object.value("elapsedSeconds").toInt(0);
    record.finishedAt = object.value("finishedAt").toString().toStdString();
    return record;
}

std::string RecordManager::recordPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("history.json").toStdString();
}

std::vector<RunRecord> RecordManager::loadRecords(std::string* error) const
{
    QFile file(QString::fromStdString(recordPath()));
    if (!file.exists()) {
        return {};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString().toStdString();
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        if (error) {
            *error = parseError.errorString().toStdString();
        }
        return {};
    }

    std::vector<RunRecord> records;
    const QJsonArray array = doc.array();
    records.reserve(array.size());
    for (const QJsonValue& value : array) {
        records.push_back(recordFromJson(value.toObject()));
    }
    return records;
}

bool RecordManager::appendRecord(const RunRecord& record, std::string* error) const
{
    std::vector<RunRecord> records = loadRecords(error);
    RunRecord saved = record;
    if (saved.finishedAt.empty()) {
        saved.finishedAt = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
    }
    records.insert(records.begin(), std::move(saved));

    QJsonArray array;
    for (const RunRecord& existing : records) {
        array.append(recordToJson(existing));
    }

    QFile file(QString::fromStdString(recordPath()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString().toStdString();
        }
        return false;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}
