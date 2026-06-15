#include "effectfactory.h"

#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "entity/component/teamcomponent.h"

//namespace前向声明，可以在其他cpp中找到这个空间；这种方式不应该再include .cpp，只是把这个函数交给其他文件写了而已，编译器会自动连接
//写的时候只要有声明就好了，调用会自己去找这个函数
namespace heal_effect{
effect::BeAttackedHandler reverseDamageToHeal();
effect::TurnHandler regenerationHeal();
}

namespace damage_effect{
effect::AttackHandler witchification();
effect::TurnHandler burningDamage();
}

namespace attribute_effect {
effect::BattleHandler statModifier();
effect::AttackHandler damageMultiplier();
effect::BeAttackedHandler damageReductionPercent();
effect::BeAttackedHandler damageImmunity();
effect::BeAttackedHandler lowHealthGuard();
effect::BeAttackedHandler attackUpAfterDamage();
effect::DeathHandler deathProtection();
}

namespace control_effect {
effect::ApplyHandler temporaryTeam(teams team);
effect::RemoveHandler clearTemporaryTeam();
effect::ApplyHandler untargetable();
effect::RemoveHandler clearUntargetable();
}

namespace character_effect {
effect::BattleHandler cocoStart();
effect::BeAttackedHandler cocoRetreat();
effect::BattleHandler levitation();
effect::BattleHandler sightInduction();
effect::RemoveHandler clearSightInduction();
effect::AttackHandler removeSightInduction();
effect::BattleHandler simpleSpearTargeting();
effect::AttackHandler firstAttackDouble();
effect::TurnHandler burningPercent();
effect::BeAttackedHandler noahDamageReduction();
effect::TurnHandler noahRegeneration();
effect::DeathHandler deathRewind();
effect::BeAttackedHandler mimicry();
}

QJsonObject effectfactory::loadEffectList()
{
    const QString path = QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath("effectlist.json");
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open effectlist.json:" << file.errorString();
        return {};
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse effectlist.json:" << error.errorString();
        return {};
    }

    if (!doc.isObject()) {
        qDebug() << "effectlist.json root must be an object";
        return {};
    }

    return doc.object();
}

const QJsonObject& effectfactory::effectList()
{
    static QJsonObject data = loadEffectList();
    return data;
}

std::unique_ptr<effect> effectfactory::create(int effectid)
{
    const QJsonArray effects = effectList().value("effects").toArray();
    for (const QJsonValue& value : effects) {
        const QJsonObject effectObject = value.toObject();
        if (effectObject.value("effectid").toInt() != effectid) {
            continue;
        }

        const std::string name = effectObject.value("name").toString().toStdString();
        const QJsonObject params = effectObject.value("params").toObject();

        auto created = std::make_unique<effect>(effectid, name, params);

        switch (effectid) {
        case 1:
            created->setBeforeBeAttacked(heal_effect::reverseDamageToHeal());
            break;
        case 2:
            created->setAfterAttack(damage_effect::witchification());
            break;
        case 3:
            created->setOnTurnStart(damage_effect::burningDamage());
            break;
        case 4:
            created->setOnTurnStart(heal_effect::regenerationHeal());
            break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 19:
        case 20:
        case 21:
        case 25:
            created->setOnBattleStart(attribute_effect::statModifier());
            break;
        case 13:
        case 17:
            created->setBeforeBeAttacked(attribute_effect::damageReductionPercent());
            break;
        case 14:
        case 18:
            created->setBeforeBeAttacked(attribute_effect::damageImmunity());
            break;
        case 15:
            created->setBeforeBeAttacked(attribute_effect::lowHealthGuard());
            break;
        case 27:
            created->setBeforeBeAttacked(attribute_effect::attackUpAfterDamage());
            break;
        case 16:
        case 22:
        case 24:
            created->setBeforeAttack(attribute_effect::damageMultiplier());
            break;
        case 23:
            created->setBeforeDeath(attribute_effect::deathProtection());
            break;
        case 201:
            created->setApply(control_effect::temporaryTeam(teams::pc));
            created->setRemove(control_effect::clearTemporaryTeam());
            break;
        case 202:
            created->setApply(control_effect::temporaryTeam(teams::enemy));
            created->setRemove(control_effect::clearTemporaryTeam());
            break;
        case 203:
            created->setApply(control_effect::untargetable());
            created->setRemove(control_effect::clearUntargetable());
            break;
        case 301:
            created->setOnBattleStart(character_effect::cocoStart());
            break;
        case 302:
            created->setAfterBeAttacked(character_effect::cocoRetreat());
            break;
        case 305:
            created->setOnBattleStart(character_effect::levitation());
            break;
        case 306:
            created->setOnBattleStart(character_effect::sightInduction());
            created->setRemove(character_effect::clearSightInduction());
            created->setAfterAttack(character_effect::removeSightInduction());
            break;
        case 308:
            created->setOnBattleStart(character_effect::simpleSpearTargeting());
            break;
        case 309:
            created->setBeforeAttack(character_effect::firstAttackDouble());
            break;
        case 310:
            created->setOnTurnStart(character_effect::burningPercent());
            break;
        case 312:
            created->setBeforeBeAttacked(character_effect::noahDamageReduction());
            break;
        case 313:
            created->setOnTurnStart(character_effect::noahRegeneration());
            break;
        case 314:
            created->setBeforeDeath(character_effect::deathRewind());
            break;
        case 315:
            created->setAfterBeAttacked(character_effect::mimicry());
            break;
        default:
            break;
        }

        return created;
    }

    qDebug() << "Unknown effect id:" << effectid;
    return nullptr;
}
