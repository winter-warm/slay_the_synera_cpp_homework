#include "defaultskill.h"
#include "character.h"
#include "combat/buff/bufffactory.h"

#include <QDebug>


Skill skillFromName(const QString& name)
{
    if(name == "ReverseFuture"){return ReverseFuture;}
    else if(name == "Savage"){return Savage;}

    qDebug() << "Unknown skill name:" << name;
    return [](Character*, Character*){};
}



void ReverseFuture(Character* caster, Character* target){
    target->getbuff().addBuff(bufffactory::create(1, -1));
}

void Savage(Character* caster,Character* target){
    target->getstats().beattacked(25);
}
