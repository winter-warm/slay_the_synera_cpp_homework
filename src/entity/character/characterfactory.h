#ifndef CHARACTERFACTORY_H
#define CHARACTERFACTORY_H

#include <memory>

#include "character.h"

#include <QJsonObject>

class characterfactory
{
private:
    static QJsonObject loadCharacterTemplates();
    static const QJsonObject& characterTemplates();

public:
    static std::unique_ptr<Character> create(int templateId);
};

#endif // CHARACTERFACTORY_H
