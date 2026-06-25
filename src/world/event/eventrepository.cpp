#include "eventrepository.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QDebug>
#include <unordered_set>

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
    config.defeatStepId = object.value("defeatStepId").toString().toStdString();
    config.poolLayerId = object.value("layer").toInt(object.value("poolLayerId").toInt());
    config.statMultiplier = static_cast<float>(object.value("statMultiplier").toDouble(1.0));
    const QJsonArray enemies = object.value("enemies").toArray();
    for (const QJsonValue& value : enemies) {
        const QJsonObject enemyObject = value.toObject();
        const QJsonObject position = enemyObject.value("position").toObject();
        EnemyPlacement enemy;
        enemy.templateId = enemyObject.value("enemy").toInt(enemyObject.value("templateId").toInt());
        enemy.hex.x = position.value("x").toInt();
        enemy.hex.y = position.value("y").toInt();
        enemy.hex.z = -enemy.hex.x - enemy.hex.y;
        if (enemy.templateId > 0 && isValidHex(enemy.hex)) {
            config.enemies.push_back(enemy);
        }
    }
    return config;
}

static EventAction actionFromJson(const QJsonObject& object) {
    EventAction action;
    const QString type = object.value("type").toString("finish");
    if (type == "startBattle") {
        action.type = EventActionType::StartBattle;
        action.battle = battleFromJson(object.value("battle").toObject());
        action.battle.returnStepId = object.value("returnStepId")
                                         .toString(QString::fromStdString(action.battle.returnStepId))
                                         .toStdString();
        action.battle.defeatStepId = object.value("defeatStepId")
                                         .toString(QString::fromStdString(action.battle.defeatStepId))
                                         .toStdString();
    } else if (type == "goToStep") {
        action.type = EventActionType::GoToStep;
        action.nextStepId = object.value("stepId").toString(object.value("nextStepId").toString("start")).toStdString();
    } else if (type == "showMessage") {
        action.type = EventActionType::ShowMessage;
    } else if (type == "lotteryScratch") {
        action.type = EventActionType::LotteryScratch;
    } else if (type == "heal") {
        action.type = EventActionType::Heal;
    } else if (type == "loseHp") {
        action.type = EventActionType::LoseHp;
    } else if (type == "changeMaxHp") {
        action.type = EventActionType::ChangeMaxHp;
    } else if (type == "grantGold") {
        action.type = EventActionType::GrantGold;
    } else if (type == "loseGold") {
        action.type = EventActionType::LoseGold;
    } else if (type == "grantRandomGold") {
        action.type = EventActionType::GrantRandomGold;
    } else if (type == "grantCard") {
        action.type = EventActionType::GrantCard;
    } else if (type == "grantRandomCard") {
        action.type = EventActionType::GrantRandomCard;
    } else if (type == "upgradeRandomOwnedCard") {
        action.type = EventActionType::UpgradeRandomOwnedCard;
    } else if (type == "grantShopExp") {
        action.type = EventActionType::GrantShopExp;
    } else if (type == "grantRandomEquipment") {
        action.type = EventActionType::GrantRandomEquipment;
    } else if (type == "chooseOwnedCard") {
        action.type = EventActionType::ChooseOwnedCard;
    } else if (type == "chooseRecruit") {
        action.type = EventActionType::ChooseRecruit;
    } else if (type == "chooseBasicEquipment") {
        action.type = EventActionType::ChooseBasicEquipment;
    } else if (type == "chooseAdvancedEquipment") {
        action.type = EventActionType::ChooseAdvancedEquipment;
    } else if (type == "upgradeSelectedOwnedCard") {
        action.type = EventActionType::UpgradeSelectedOwnedCard;
    } else {
        action.type = EventActionType::FinishNode;
    }
    action.title = object.value("title").toString().toStdString();
    action.message = object.value("message").toString().toStdString();
    action.winText = object.value("winText").toString().toStdString();
    action.loseText = object.value("loseText").toString().toStdString();
    action.evenText = object.value("evenText").toString().toStdString();
    action.resultSuffix = object.value("resultSuffix").toString().toStdString();
    action.prompt = object.value("prompt").toString().toStdString();
    action.filter = object.value("filter").toString().toStdString();
    action.amount = object.value("amount").toInt();
    action.minAmount = object.value("min").toInt(object.value("minAmount").toInt());
    action.maxAmount = object.value("max").toInt(object.value("maxAmount").toInt());
    action.templateId = object.value("templateId").toInt(object.value("cardId").toInt());
    const QJsonArray onChoose = object.value("onChoose").toArray();
    for (const QJsonValue& value : onChoose) {
        action.onChooseActions.push_back(actionFromJson(value.toObject()));
    }
    const QJsonArray payouts = object.value("payouts").toArray();
    for (const QJsonValue& value : payouts) {
        const QJsonObject payoutObject = value.toObject();
        LotteryPayout payout;
        payout.value = payoutObject.value("value").toInt();
        payout.weight = payoutObject.value("weight").toInt(1);
        if (payout.weight < 1) {
            payout.weight = 1;
        }
        action.lotteryPayouts.push_back(payout);
    }
    return action;
}

static EventRequirement requirementFromJson(const QJsonObject& object) {
    EventRequirement requirement;
    requirement.type = object.value("type").toString().toStdString();
    requirement.value = object.value("value").toString().toStdString();
    requirement.amount = object.value("amount").toInt();
    requirement.templateId = object.value("templateId").toInt(object.value("cardId").toInt());
    return requirement;
}

static EventOption optionFromJson(const QJsonObject& object) {
    EventOption option;
    option.label = object.value("label").toString("Continue").toStdString();
    option.description = object.value("description").toString().toStdString();
    option.requiresGold = object.contains("requiresGold") ? object.value("requiresGold").toInt() : -1;
    option.requiresHpAbove = object.contains("requiresHpAbove") ? object.value("requiresHpAbove").toInt() : -1;
    const QJsonArray requirements = object.value("requirements").toArray();
    for (const QJsonValue& value : requirements) {
        option.requirements.push_back(requirementFromJson(value.toObject()));
    }
    const QJsonArray actions = object.value("actions").toArray();
    for (const QJsonValue& value : actions) {
        option.actions.push_back(actionFromJson(value.toObject()));
    }
    if (option.actions.empty()) {
        option.actions.push_back(EventAction{});
    }
    return option;
}

static EventDefinition eventFromJson(int eventId,
                                     const std::string& stepId,
                                     const QJsonObject& eventObject,
                                     const QJsonObject& stepObject) {
    EventDefinition event;
    event.id = eventId;
    event.stepId = stepId;
    event.title = stepObject.value("title").toString(eventObject.value("title").toString("Event")).toStdString();
    event.backgroundPath =
        stepObject.value("backgroundPath").toString(eventObject.value("backgroundPath").toString()).toStdString();
    event.text = stepObject.value("text").toString().toStdString();
    const QJsonArray options = stepObject.value("options").toArray();
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

static void collectGoToSteps(const QJsonArray& actions, std::vector<QString>* stepIds) {
    for (const QJsonValue& value : actions) {
        const QJsonObject action = value.toObject();
        if (action.value("type").toString() == "goToStep") {
            stepIds->push_back(action.value("stepId").toString(action.value("nextStepId").toString("start")));
        }
        collectGoToSteps(action.value("onChoose").toArray(), stepIds);
    }
}

static void validateEventTable(const QJsonObject& root) {
    const QJsonObject events = root.value("events").toObject();
    for (auto eventIt = events.begin(); eventIt != events.end(); ++eventIt) {
        const QString eventId = eventIt.key();
        const QJsonObject event = eventIt.value().toObject();
        const QJsonObject steps = event.value("steps").toObject();
        std::unordered_set<std::string> stepIds;
        for (auto stepIt = steps.begin(); stepIt != steps.end(); ++stepIt) {
            stepIds.insert(stepIt.key().toStdString());
        }

        for (auto stepIt = steps.begin(); stepIt != steps.end(); ++stepIt) {
            const QJsonObject step = stepIt.value().toObject();
            const QJsonArray options = step.value("options").toArray();
            if (eventId != "2" && options.size() > 0 && options.size() > 4) {
                qWarning() << "Event" << eventId << "step" << stepIt.key()
                           << "should have 1-4 options for the normal event template.";
            }
            std::vector<QString> referencedSteps;
            for (const QJsonValue& optionValue : options) {
                collectGoToSteps(optionValue.toObject().value("actions").toArray(), &referencedSteps);
            }
            for (const QString& referencedStep : referencedSteps) {
                if (stepIds.find(referencedStep.toStdString()) == stepIds.end()) {
                    qWarning() << "Event" << eventId << "step" << stepIt.key()
                               << "references missing step" << referencedStep;
                }
            }
        }
    }
}

EventRepository::EventRepository()
    : root(loadRoot()) {
    validateEventTable(root);
}

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
    return eventFromJson(context.eventId, resolvedStepId, event, step);
}
