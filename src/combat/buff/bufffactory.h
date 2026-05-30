#ifndef BUFFFACTORY_H
#define BUFFFACTORY_H
#include <memory>
#include "buff.h"
#include <QFile>//用QFile读json比fstream更方便，Qt有处理json的库
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

class bufffactory
{
private:
    static QJsonObject loadBuffList();
    static const QJsonObject& buffList();
public:
    bufffactory()=default;
    static std::unique_ptr<buff> create(int buffid,int duration);
};

#endif // BUFFFACTORY_H
