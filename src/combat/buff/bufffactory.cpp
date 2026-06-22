#include "bufffactory.h"

#include "../effect/effectfactory.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>

QJsonObject bufffactory::loadBuffList(){
    const QString path = QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath("bufflist.json");
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open bufflist.json:" << file.errorString();
        return {};
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse bufflist.json:" << error.errorString();
        return {};
    }

    if (!doc.isObject()) {
        qDebug() << "bufflist.json root must be an object";
        return {};
    }

    return doc.object();
}

const QJsonObject& bufffactory::buffList(){
    static QJsonObject data = loadBuffList();
    return data;
}

std::unique_ptr<buff> bufffactory::create(int buffid, int duration){
    const QJsonArray buffs = buffList().value("buffs").toArray();

    for (const QJsonValue& value : buffs) {
        const QJsonObject buffObject = value.toObject();
        if (buffObject.value("buffid").toInt() != buffid) {
            continue;//不匹配
        }

        //匹配，创建
        std::vector<std::unique_ptr<effect>> effects;
        const QJsonArray effectList = buffObject.value("effects").toArray();
        for (const QJsonValue& effectValue : effectList) {
            const int effectid = effectValue.toInt();
            std::unique_ptr<effect> created = effectfactory::create(effectid);
            if (created) {
                effects.push_back(std::move(created));
            }
        }

        float cooldown = 0.0f;//f是字面量，表示float，而不是double
        if (buffObject.contains("cooldown")) {
            cooldown = static_cast<float>(buffObject.value("cooldown").toDouble());
        }

        const int maxTriggers = buffObject.value("maxTriggers").toInt(-1);
        BuffTrigger trigger = BuffTrigger::None;
        const QString triggerName = buffObject.value("trigger").toString();
        if(triggerName == "onBattleStart"){
            trigger = BuffTrigger::OnBattleStart;
        }else if(triggerName == "beforeAttack"){
            trigger = BuffTrigger::BeforeAttack;
        }else if(triggerName == "afterAttack"){
            trigger = BuffTrigger::AfterAttack;
        }else if(triggerName == "beforeBeAttacked"){
            trigger = BuffTrigger::BeforeBeAttacked;
        }else if(triggerName == "afterBeAttacked"){
            trigger = BuffTrigger::AfterBeAttacked;
        }else if(triggerName == "beforeHeal"){
            trigger = BuffTrigger::BeforeHeal;
        }else if(triggerName == "afterSkill"){
            trigger = BuffTrigger::AfterSkill;
        }else if(triggerName == "beforeAddBuff"){
            trigger = BuffTrigger::BeforeAddBuff;
        }else if(triggerName == "beforeDeath"){
            trigger = BuffTrigger::BeforeDeath;
        }else if(!triggerName.isEmpty()){
            qDebug() << "Unsupported buff trigger:" << triggerName;
        }

        return std::make_unique<buff>(buffid,buffObject.value("name").toString(),duration,cooldown,maxTriggers,trigger,std::move(effects),
            static_cast<short>(buffObject.value("priority").toInt()));
    }

    qDebug() << "Unknown buff id:" << buffid;//没找到
    return nullptr;
}
