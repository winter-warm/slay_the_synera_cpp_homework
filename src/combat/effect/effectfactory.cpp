#include "effectfactory.h"

#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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
        default:
            break;
        }

        return created;
    }

    qDebug() << "Unknown effect id:" << effectid;
    return nullptr;
}
