#ifndef CHARACTERFACTORY_H
#define CHARACTERFACTORY_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "character.h"

#include <QJsonObject>

class characterfactory
{
private:
    static QJsonObject loadCharacterTemplates();
    static const QJsonObject& characterTemplates();

public:
    static std::unique_ptr<Character> create(int templateId);
    struct CharacterTemplateInfo {
        int templateId = 0;
        std::string name;
        std::string displayName;
        std::string magicName;
        std::string skillDescription;
        int rarity = 1;
        int maxHp = 100;
        int attack = 10;
        int defense = 0;
        bool playerCharacter = true;
    };
    static std::optional<CharacterTemplateInfo> infoFor(int templateId);
    static std::vector<CharacterTemplateInfo> playerTemplates();
};

#endif // CHARACTERFACTORY_H
