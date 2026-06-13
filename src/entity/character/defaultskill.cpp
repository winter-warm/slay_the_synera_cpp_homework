#include "defaultskill.h"
#include "character.h"
#include "combat/buff/bufffactory.h"

#include <QDebug>


Skill skillFromName(const QString& name)
{
    if(name == "ReverseFuture"){return ReverseFuture;}
    else if(name == "Savage"){return Savage;}
    else if(name == "PawnAdvance"){return PawnAdvance;}
    else if(name == "CastleWall"){return CastleWall;}
    else if(name == "KnightFork"){return KnightFork;}
    else if(name == "DiagonalSmite"){return DiagonalSmite;}
    else if(name == "RoyalSweep"){return RoyalSweep;}
    else if(name == "KingCommand"){return KingCommand;}

    qDebug() << "Unknown skill name:" << name;
    return [](Character*, Character*){};
}

static bool invalidSkillTarget(Character* caster, Character* target){
    return caster == nullptr || target == nullptr;
}



void ReverseFuture(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getbuff().addBuff(bufffactory::create(1, -1));
}

void Savage(Character* caster,Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(25);
}

void PawnAdvance(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(18, caster);
}

void CastleWall(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    caster->getstats().modifySHIELF(18);
    target->getstats().beattacked(12, caster);
}

void KnightFork(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(28, caster);
}

void DiagonalSmite(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(16, caster);
    target->getbuff().addBuff(bufffactory::create(2, 2));
}

void RoyalSweep(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(34, caster);
    caster->getstats().modifyHP(-8);
}

void KingCommand(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    caster->getstats().modifyHP(18);
    caster->getstats().modifySHIELF(12);
    target->getstats().beattacked(14, caster);
}
