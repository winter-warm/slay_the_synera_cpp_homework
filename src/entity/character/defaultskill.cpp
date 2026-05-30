#include "defaultskill.h"
#include "character.h"
#include "combat/buff/bufffactory.h"

#include <QDebug>

void ReverseFuture(Character* caster, Character* target){
    target->getbuff().addBuff(bufffactory::create(1, -1));
}

Skill skillFromName(const QString& name)
{
    if(name == "ReverseFuture"){
        return ReverseFuture;
    }

    qDebug() << "Unknown skill name:" << name;
    return [](Character*, Character*){};
}
