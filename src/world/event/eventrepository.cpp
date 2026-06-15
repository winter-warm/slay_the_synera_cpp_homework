#include "eventrepository.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

static QJsonObject loadRoot() {
    const QString runtimePath = QCoreApplication::applicationDirPath() + "/events/events.json";
    const QString sourcePath = QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath("events.json");

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

static BattleKind battleKindFromString(const QString& value) {
    if (value == "normal") {
        return BattleKind::Normal;
    }
    if (value == "elite") {
        return BattleKind::Elite;
    }
    if (value == "boss") {
        return BattleKind::Boss;
    }
    return BattleKind::None;
}

static BattleConfig battleFromJson(const QJsonObject& object) {
    BattleConfig config;
    config.kind = battleKindFromString(object.value("kind").toString());
    config.boardId = object.value("boardId").toString("default_board").toStdString();
    config.returnStepId = object.value("returnStepId").toString().toStdString();
    return config;
}

static EventAction actionFromJson(const QJsonObject& object) {
    EventAction action;
    const QString type = object.value("type").toString("finish");
    if (type == "startBattle") {
        action.type = EventActionType::StartBattle;
        action.battle = battleFromJson(object.value("battle").toObject());
        action.battle.returnStepId = object.value("returnStepId").toString().toStdString();
    } else if (type == "goToStep") {
        action.type = EventActionType::GoToStep;
        action.nextStepId = object.value("stepId").toString("start").toStdString();
    } else {
        action.type = EventActionType::FinishNode;
    }
    return action;
}

static EventOption optionFromJson(const QJsonObject& object) {
    EventOption option;
    option.label = object.value("label").toString("Continue").toStdString();
    const QJsonArray actions = object.value("actions").toArray();
    for (const QJsonValue& value : actions) {
        option.actions.push_back(actionFromJson(value.toObject()));
    }
    if (option.actions.empty()) {
        option.actions.push_back(EventAction{});
    }
    return option;
}

static EventDefinition eventFromJson(int eventId, const std::string& stepId, const QJsonObject& object) {
    EventDefinition event;
    event.id = eventId;
    event.stepId = stepId;
    event.title = object.value("title").toString("Event").toStdString();
    event.backgroundPath = object.value("backgroundPath").toString().toStdString();
    event.text = object.value("text").toString().toStdString();
    const QJsonArray options = object.value("options").toArray();
    for (const QJsonValue& value : options) {
        event.options.push_back(optionFromJson(value.toObject()));
    }
    if (event.options.empty()) {
        EventOption option;
        option.label = "Finish Event";
        option.actions.push_back(EventAction{});
        event.options.push_back(option);
    }
    return event;
}

EventRepository::EventRepository()
    : root(loadRoot()) {}

std::optional<EventDefinition> EventRepository::eventFor(const EventContext& context,
                                                         const std::string& stepId) const {
    const QJsonObject events = root.value("events").toObject();
    QJsonObject event = events.value(QString::number(context.eventId)).toObject();
    if (event.isEmpty()) {
        event = events.value("default").toObject();
    }
    if (event.isEmpty()) {
        return std::nullopt;
    }

    const QJsonObject steps = event.value("steps").toObject();
    QJsonObject step = steps.value(QString::fromStdString(stepId)).toObject();
    std::string resolvedStepId = stepId;
    if (step.isEmpty()) {
        step = steps.value("start").toObject();
        resolvedStepId = "start";
    }
    if (step.isEmpty()) {
        return std::nullopt;
    }
    return eventFromJson(context.eventId, resolvedStepId, step);
}
