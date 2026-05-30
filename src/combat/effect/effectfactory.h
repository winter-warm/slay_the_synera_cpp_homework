#ifndef EFFECTFACTORY_H
#define EFFECTFACTORY_H

#include <memory>

#include "effect.h"

#include <QJsonObject>

class effectfactory
{
private:
    static QJsonObject loadEffectList();
    static const QJsonObject& effectList();

public:
    static std::unique_ptr<effect> create(int effectid);
};

#endif // EFFECTFACTORY_H
