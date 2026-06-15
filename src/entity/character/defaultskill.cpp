#include "defaultskill.h"
#include "character.h"
#include "core/board.h"
#include "combat/buff/bufffactory.h"
#include "entity/component/movecomponent.h"

#include <QDebug>
#include <algorithm>
#include <random>
#include <vector>


Skill skillFromName(const QString& name, bool isBuff)
{
    if(name == "ReverseFuture"){return {ReverseFuture, isBuff};}
    else if(name == "Savage"){return {Savage, isBuff};}
    else if(name == "PawnAdvance"){return {PawnAdvance, isBuff};}
    else if(name == "CastleWall"){return {CastleWall, isBuff};}
    else if(name == "KnightFork"){return {KnightFork, isBuff};}
    else if(name == "DiagonalSmite"){return {DiagonalSmite, isBuff};}
    else if(name == "RoyalSweep"){return {RoyalSweep, isBuff};}
    else if(name == "KingCommand"){return {KingCommand, isBuff};}
    else if(name == "KillingIntentSense"){return {KillingIntentSense, isBuff};}
    else if(name == "EhEhOh"){return {EhEhOh, isBuff};}
    else if(name == "Levitation"){return {Levitation, isBuff};}
    else if(name == "SightInduction"){return {SightInduction, isBuff};}
    else if(name == "SimpleSpear"){return {SimpleSpear, isBuff};}
    else if(name == "Fireball"){return {Fireball, isBuff};}
    else if(name == "Kotodama"){return {Kotodama, isBuff};}
    else if(name == "LiquidManipulation"){return {LiquidManipulation, isBuff};}
    else if(name == "Gunshot"){return {Gunshot, isBuff};}
    else if(name == "DeathRewind"){return {DeathRewind, isBuff};}
    else if(name == "Mimicry"){return {Mimicry, isBuff};}
    else if(name == "WitchKiller"){return {WitchKiller, isBuff};}

    qDebug() << "Unknown skill name:" << name;
    return {};
}

static bool invalidSkillTarget(Character* caster, Character* target){
    return caster == nullptr || target == nullptr;
}

static std::mt19937& defaultSkillRng()
{
    static std::mt19937 rng{std::random_device{}()};
    return rng;
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
    caster->getstats().heal(18, caster);
    caster->getstats().modifySHIELF(12);
    target->getstats().beattacked(14, caster);
}

void KillingIntentSense(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(301, -1));
}

void EhEhOh(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(target->getstats().getHP(), caster);
}

void Levitation(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(305, -1));
}

void SightInduction(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(306, 15));
}

void SimpleSpear(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(308, -1));
    caster->getbuff().addBuff(bufffactory::create(309, -1));
}

void Fireball(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(10, caster);
    target->getbuff().addBuff(bufffactory::create(310, 3));
}

void Kotodama(Character* caster, Character* target){
    if(caster == nullptr){
        return;
    }

    Board* board = caster->getCurrentBoard();
    Character* selected = board ? nearestEnemy(caster, *board) : target;
    if(selected == nullptr){
        return;
    }

    const int buffId = selected->getteam().getteam() == teams::enemy ? 201 : 202;
    selected->getbuff().addBuff(bufffactory::create(buffId, 10));
}

void LiquidManipulation(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(312, -1));
    caster->getbuff().addBuff(bufffactory::create(313, -1));
}

void Gunshot(Character* caster, Character* target){
    if(invalidSkillTarget(caster, target)){
        return;
    }
    target->getstats().beattacked(30, caster);
}

void DeathRewind(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(314, -1));
}

void Mimicry(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr){
        return;
    }
    caster->getbuff().addBuff(bufffactory::create(315, -1));
}

void WitchKiller(Character* caster, Character* target){
    (void)target;
    if(caster == nullptr || caster->getCurrentBoard() == nullptr){
        return;
    }

    Board* board = caster->getCurrentBoard();
    std::vector<Character*> characters;
    std::vector<Hex> positions;
    for(Unit* unit : board->units()){
        Character* character = dynamic_cast<Character*>(unit);
        Hex hex;
        if(character && board->posOf(character, &hex)){
            character->getstats().modifySHIELF(-character->getstats().getSHIELD());
            character->getstats().setHP(1);
            characters.push_back(character);
            positions.push_back(hex);
        }
    }
    if(characters.size() < 2){
        return;
    }

    std::shuffle(positions.begin(), positions.end(), defaultSkillRng());
    for(Character* character : characters){
        board->remove(character);
    }
    for(size_t i = 0; i < characters.size(); ++i){
        board->add(characters[i], positions[i]);
    }
}
