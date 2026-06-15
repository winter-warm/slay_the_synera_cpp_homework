#include "gamestate.h"
#include <QJsonArray>
#include <QString>
#include <vector>

static QJsonArray intsToJson(const std::vector<int>& values) {
    QJsonArray array;
    for (int value : values) {
        array.append(value);
    }
    return array;
}

static std::vector<int> intsFromJson(const QJsonArray& array) {
    std::vector<int> values;
    for (const QJsonValue& value : array) {
        values.push_back(value.toInt());
    }
    return values;
}

static QString battleKindToString(BattleKind kind) {
    switch (kind) {
    case BattleKind::Normal:
        return "normal";
    case BattleKind::Elite:
        return "elite";
    case BattleKind::Boss:
        return "boss";
    case BattleKind::None:
        return "none";
    }
    return "none";
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

static QString actionTypeToString(EventActionType type) {
    switch (type) {
    case EventActionType::StartBattle:
        return "startBattle";
    case EventActionType::GoToStep:
        return "goToStep";
    case EventActionType::FinishNode:
        return "finish";
    }
    return "finish";
}

static EventActionType actionTypeFromString(const QString& value) {
    if (value == "startBattle") {
        return EventActionType::StartBattle;
    }
    if (value == "goToStep") {
        return EventActionType::GoToStep;
    }
    return EventActionType::FinishNode;
}

static QJsonObject battleToJson(const BattleConfig& battle) {
    QJsonObject object;
    object["kind"] = battleKindToString(battle.kind);
    object["boardId"] = QString::fromStdString(battle.boardId);
    QJsonArray enemies;
    for (const EnemyPlacement& enemy : battle.enemies) {
        QJsonObject enemyObject;
        enemyObject["enemy"] = enemy.templateId;
        QJsonObject position;
        position["x"] = enemy.hex.x;
        position["y"] = enemy.hex.y;
        enemyObject["position"] = position;
        enemies.append(enemyObject);
    }
    object["enemies"] = enemies;
    object["statMultiplier"] = battle.statMultiplier;
    object["returnStepId"] = QString::fromStdString(battle.returnStepId);
    return object;
}

static BattleConfig battleFromJson(const QJsonObject& object) {
    BattleConfig battle;
    battle.kind = battleKindFromString(object.value("kind").toString());
    battle.boardId = object.value("boardId").toString("default_board").toStdString();
    battle.statMultiplier = static_cast<float>(object.value("statMultiplier").toDouble(1.0));
    const QJsonArray enemies = object.value("enemies").toArray();
    for (const QJsonValue& value : enemies) {
        const QJsonObject enemyObject = value.toObject();
        const QJsonObject position = enemyObject.value("position").toObject();
        EnemyPlacement enemy;
        enemy.templateId = enemyObject.value("enemy").toInt();
        enemy.hex.x = position.value("x").toInt();
        enemy.hex.y = position.value("y").toInt();
        enemy.hex.z = -enemy.hex.x - enemy.hex.y;
        if (enemy.templateId > 0 && isValidHex(enemy.hex)) {
            battle.enemies.push_back(enemy);
        }
    }
    battle.returnStepId = object.value("returnStepId").toString().toStdString();
    return battle;
}

static QJsonObject actionToJson(const EventAction& action) {
    QJsonObject object;
    object["type"] = actionTypeToString(action.type);
    object["nextStepId"] = QString::fromStdString(action.nextStepId);
    object["battle"] = battleToJson(action.battle);
    return object;
}

static EventAction actionFromJson(const QJsonObject& object) {
    EventAction action;
    action.type = actionTypeFromString(object.value("type").toString());
    action.nextStepId = object.value("nextStepId").toString().toStdString();
    action.battle = battleFromJson(object.value("battle").toObject());
    return action;
}

static QJsonObject optionToJson(const EventOption& option) {
    QJsonObject object;
    object["label"] = QString::fromStdString(option.label);
    QJsonArray actions;
    for (const EventAction& action : option.actions) {
        actions.append(actionToJson(action));
    }
    object["actions"] = actions;
    return object;
}

static EventOption optionFromJson(const QJsonObject& object) {
    EventOption option;
    option.label = object.value("label").toString().toStdString();
    const QJsonArray actions = object.value("actions").toArray();
    for (const QJsonValue& value : actions) {
        option.actions.push_back(actionFromJson(value.toObject()));
    }
    return option;
}

static QJsonObject currentEventToJson(const CurrentEventState& event) {
    QJsonObject object;
    object["active"] = event.active;
    object["eventId"] = event.eventId;
    object["stepId"] = QString::fromStdString(event.stepId);
    object["title"] = QString::fromStdString(event.title);
    object["backgroundPath"] = QString::fromStdString(event.backgroundPath);
    object["text"] = QString::fromStdString(event.text);
    object["hexTechSelection"] = event.hexTechSelection;
    QJsonArray options;
    for (const EventOption& option : event.options) {
        options.append(optionToJson(option));
    }
    object["options"] = options;
    return object;
}

static CurrentEventState currentEventFromJson(const QJsonObject& object) {
    CurrentEventState event;
    event.active = object.value("active").toBool(false);
    event.eventId = object.value("eventId").toInt(-1);
    event.stepId = object.value("stepId").toString("start").toStdString();
    event.title = object.value("title").toString().toStdString();
    event.backgroundPath = object.value("backgroundPath").toString().toStdString();
    event.text = object.value("text").toString().toStdString();
    event.hexTechSelection = object.value("hexTechSelection").toBool(false);
    const QJsonArray options = object.value("options").toArray();
    for (const QJsonValue& value : options) {
        event.options.push_back(optionFromJson(value.toObject()));
    }
    return event;
}

static QJsonObject hexTechEffectToJson(const HexTechEffect& effect) {
    QJsonObject object;
    object["type"] = QString::fromStdString(effect.type);
    object["value"] = effect.value;
    object["buffId"] = effect.buffId;
    object["duration"] = effect.duration;
    object["modifierKey"] = QString::fromStdString(effect.modifierKey);
    return object;
}

static HexTechEffect hexTechEffectFromJson(const QJsonObject& object) {
    HexTechEffect effect;
    effect.type = object.value("type").toString().toStdString();
    effect.value = object.value("value").toInt();
    effect.buffId = object.value("buffId").toInt();
    effect.duration = object.value("duration").toInt();
    effect.modifierKey = object.value("modifierKey").toString().toStdString();
    return effect;
}

static QJsonObject hexTechDefinitionToJson(const HexTechDefinition& definition) {
    QJsonObject object;
    object["id"] = QString::fromStdString(definition.id);
    object["layerId"] = definition.layerId;
    object["name"] = QString::fromStdString(definition.name);
    object["title"] = QString::fromStdString(definition.title);
    object["description"] = QString::fromStdString(definition.description);
    object["cardImagePath"] = QString::fromStdString(definition.cardImagePath);
    QJsonArray effects;
    for (const HexTechEffect& effect : definition.effects) {
        effects.append(hexTechEffectToJson(effect));
    }
    object["effects"] = effects;
    return object;
}

static HexTechDefinition hexTechDefinitionFromJson(const QJsonObject& object) {
    HexTechDefinition definition;
    definition.id = object.value("id").toString().toStdString();
    definition.layerId = object.value("layerId").toInt(1);
    definition.name = object.value("name").toString().toStdString();
    definition.title = object.value("title").toString().toStdString();
    definition.description = object.value("description").toString().toStdString();
    definition.cardImagePath = object.value("cardImagePath").toString().toStdString();
    const QJsonArray effects = object.value("effects").toArray();
    for (const QJsonValue& value : effects) {
        definition.effects.push_back(hexTechEffectFromJson(value.toObject()));
    }
    return definition;
}

static QJsonObject hexTechCardRecordToJson(const HexTechCardRecord& record) {
    QJsonObject object;
    object["hexTechId"] = QString::fromStdString(record.hexTechId);
    object["layerId"] = record.layerId;
    object["title"] = QString::fromStdString(record.title);
    object["description"] = QString::fromStdString(record.description);
    object["cardImagePath"] = QString::fromStdString(record.cardImagePath);
    object["pickedAtNodeId"] = record.pickedAtNodeId;
    return object;
}

static HexTechCardRecord hexTechCardRecordFromJson(const QJsonObject& object) {
    HexTechCardRecord record;
    record.hexTechId = object.value("hexTechId").toString().toStdString();
    record.layerId = object.value("layerId").toInt(1);
    record.title = object.value("title").toString().toStdString();
    record.description = object.value("description").toString().toStdString();
    record.cardImagePath = object.value("cardImagePath").toString().toStdString();
    record.pickedAtNodeId = object.value("pickedAtNodeId").toInt(-1);
    return record;
}

static QJsonObject ownedCharacterCardToJson(const OwnedCharacterCard& card) {
    QJsonObject object;
    object["templateId"] = card.templateId;
    object["starLevel"] = card.starLevel;
    return object;
}

static OwnedCharacterCard ownedCharacterCardFromJson(const QJsonObject& object) {
    OwnedCharacterCard card;
    card.templateId = object.value("templateId").toInt();
    card.starLevel = object.value("starLevel").toInt(1);
    if (card.starLevel < 1) {
        card.starLevel = 1;
    }
    if (card.starLevel > 3) {
        card.starLevel = 3;
    }
    return card;
}

static QJsonObject shopOfferToJson(const ShopOffer& offer) {
    QJsonObject object;
    object["templateId"] = offer.templateId;
    object["sold"] = offer.sold;
    return object;
}

static ShopOffer shopOfferFromJson(const QJsonObject& object) {
    ShopOffer offer;
    offer.templateId = object.value("templateId").toInt();
    offer.sold = object.value("sold").toBool(false);
    return offer;
}

static QJsonObject shopStateToJson(const ShopState& shop) {
    QJsonObject object;
    object["level"] = shop.level;
    object["exp"] = shop.exp;
    object["rollCounter"] = shop.rollCounter;
    QJsonArray offers;
    for (const ShopOffer& offer : shop.offers) {
        offers.append(shopOfferToJson(offer));
    }
    object["offers"] = offers;
    return object;
}

static ShopState shopStateFromJson(const QJsonObject& object) {
    ShopState shop;
    shop.level = object.value("level").toInt(1);
    shop.exp = object.value("exp").toInt(0);
    shop.rollCounter = object.value("rollCounter").toInt(0);
    if (shop.level < 1) {
        shop.level = 1;
    }
    if (shop.level > 4) {
        shop.level = 4;
    }
    if (shop.exp < 0) {
        shop.exp = 0;
    }
    const QJsonArray offers = object.value("offers").toArray();
    for (const QJsonValue& value : offers) {
        shop.offers.push_back(shopOfferFromJson(value.toObject()));
    }
    return shop;
}

static QJsonArray stringsToJson(const std::vector<std::string>& values) {
    QJsonArray array;
    for (const std::string& value : values) {
        array.append(QString::fromStdString(value));
    }
    return array;
}

static std::vector<std::string> stringsFromJson(const QJsonArray& array) {
    std::vector<std::string> values;
    for (const QJsonValue& value : array) {
        values.push_back(value.toString().toStdString());
    }
    return values;
}

QJsonObject GameState::toJson() const {
    QJsonObject object;
    object["seed"] = seed;
    object["playerHp"] = playerHp;
    object["maxPlayerHp"] = maxPlayerHp;
    object["gold"] = gold;
    object["currentLayerId"] = currentLayerId;
    object["playerNodeId"] = playerNodeId;
    object["currentNodeId"] = currentNodeId;
    object["completedNodeIds"] = intsToJson(completedNodeIds);
    object["availableNodeIds"] = intsToJson(availableNodeIds);
    object["map"] = map.toJson();
    object["currentEvent"] = currentEventToJson(currentEvent);
    object["currentBattle"] = battleToJson(currentBattle);
    object["pendingEventStepAfterBattle"] = QString::fromStdString(pendingEventStepAfterBattle);
    QJsonArray selectedCards;
    for (const HexTechCardRecord& record : selectedHexTechCards) {
        selectedCards.append(hexTechCardRecordToJson(record));
    }
    object["selectedHexTechCards"] = selectedCards;
    object["activeAuraIds"] = stringsToJson(activeAuraIds);
    QJsonArray choices;
    for (const HexTechDefinition& definition : currentHexTechChoices) {
        choices.append(hexTechDefinitionToJson(definition));
    }
    object["currentHexTechChoices"] = choices;
    QJsonArray ownedCards;
    for (const OwnedCharacterCard& card : ownedCharacterCards) {
        ownedCards.append(ownedCharacterCardToJson(card));
    }
    object["ownedCharacterCards"] = ownedCards;
    object["shop"] = shopStateToJson(shop);
    return object;
}

GameState GameState::fromJson(const QJsonObject& object) {
    GameState state;
    state.seed = object.value("seed").toInt();
    state.playerHp = object.value("playerHp").toInt(80);
    state.maxPlayerHp = object.value("maxPlayerHp").toInt(80);
    state.gold = object.value("gold").toInt();
    state.currentLayerId = object.value("currentLayerId").toInt(1);
    state.playerNodeId = object.value("playerNodeId").toInt(-1);
    state.currentNodeId = object.value("currentNodeId").toInt(-1);
    state.completedNodeIds = intsFromJson(object.value("completedNodeIds").toArray());
    state.availableNodeIds = intsFromJson(object.value("availableNodeIds").toArray());
    state.map = MapGraph::fromJson(object.value("map").toObject());
    state.currentEvent = currentEventFromJson(object.value("currentEvent").toObject());
    state.currentBattle = battleFromJson(object.value("currentBattle").toObject());
    state.pendingEventStepAfterBattle = object.value("pendingEventStepAfterBattle").toString().toStdString();
    const QJsonArray selectedCards = object.value("selectedHexTechCards").toArray();
    for (const QJsonValue& value : selectedCards) {
        state.selectedHexTechCards.push_back(hexTechCardRecordFromJson(value.toObject()));
    }
    state.activeAuraIds = stringsFromJson(object.value("activeAuraIds").toArray());
    const QJsonArray choices = object.value("currentHexTechChoices").toArray();
    for (const QJsonValue& value : choices) {
        state.currentHexTechChoices.push_back(hexTechDefinitionFromJson(value.toObject()));
    }
    const QJsonArray ownedCards = object.value("ownedCharacterCards").toArray();
    for (const QJsonValue& value : ownedCards) {
        OwnedCharacterCard card = ownedCharacterCardFromJson(value.toObject());
        if (card.templateId > 0) {
            state.ownedCharacterCards.push_back(card);
        }
    }
    if (state.ownedCharacterCards.empty()) {
        state.ownedCharacterCards.push_back({1209, 1});
    }
    state.shop = shopStateFromJson(object.value("shop").toObject());
    return state;
}
