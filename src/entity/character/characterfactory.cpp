#include "characterfactory.h"

#include "defaultskill.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>

static Bond bondFromString(const QString& value){
    if(value == "soldier"){return Bond::soldier;}
    if(value == "mage"){return Bond::mage;}
    if(value == "animal"){return Bond::animal;}
    if(value == "building"){return Bond::building;}
    if(value == "pastor"){return Bond::pastor;}
    if(value == "witch"){return Bond::witch;}
    if(value == "beast"){return Bond::beast;}
    if(value == "chess"){return Bond::chess;}
    return Bond::none;
}

static teams teamFromString(const QString& value){
    if(value == "enemy"){return teams::enemy;}
    return teams::pc;
}

QJsonObject characterfactory::loadCharacterTemplates(){
    const QString path = QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath("template.json");
    QFile file(path);

    if(!file.open(QIODevice::ReadOnly)){
        qDebug() << "Failed to open template.json:" << file.errorString();
        return {};
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if(error.error != QJsonParseError::NoError){
        qDebug() << "Failed to parse template.json:" << error.errorString();
        return {};
    }

    if(!doc.isObject()){
        qDebug() << "template.json root must be an object";
        return {};
    }

    return doc.object();
}

const QJsonObject& characterfactory::characterTemplates(){
    static QJsonObject data = loadCharacterTemplates();
    return data;
}

std::unique_ptr<Character> characterfactory::create(int templateId){
    const QJsonArray characters = characterTemplates().value("characters").toArray();
    for(const QJsonValue& value : characters){
        const QJsonObject characterObject = value.toObject();
        if(characterObject.value("templateId").toInt() != templateId){
            continue;
        }

        std::array<Bond, 2> bonds{Bond::none, Bond::none};
        int bondCount = 0;
        const QJsonArray bondValues = characterObject.value("bonds").toArray();
        for(const QJsonValue& bondValue : bondValues){
            if(bondCount >= 2){
                break;
            }
            bonds[bondCount++] = bondFromString(bondValue.toString());
        }
        if(bondCount == 0){
            bondCount = 1;
        }

        std::vector<Skill> skills;
        const QJsonArray skillValues = characterObject.value("defaultSkills").toArray();
        for(const QJsonValue& skillValue : skillValues){
            skills.push_back(skillFromName(skillValue.toString()));
        }

        const QJsonObject stats = characterObject.value("baseStats").toObject();
        return std::make_unique<Character>(
            characterObject.value("name").toString().toStdString(),
            bonds,
            bondCount,
            std::move(skills),
            stats.value("maxHp").toInt(100),
            stats.value("maxMp").toInt(3),
            stats.value("attack").toInt(10),
            stats.value("defense").toInt(0),
            stats.value("range").toInt(1),
            stats.value("shield").toInt(0),
            teamFromString(characterObject.value("team").toString()));
    }

    qDebug() << "Unknown character template id:" << templateId;
    return nullptr;
}
