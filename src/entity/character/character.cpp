#include "character.h"

static int normalizeBondCount(int count){
    if (count < 1) {
        return 1;
    }
    if (count > 2) {
        return 2;
    }
    return count;
}

Character::Character(std::string name,const std::array<Bond, 2>& characterBonds,int characterBondCount,std::vector<Skill> skills,int maxhp,int maxmp,int attackValue,int defense,int range,int shield,teams characterTeam)
    : Unit(name), bonds(characterBonds), bondCount(normalizeBondCount(characterBondCount)), stats(this, maxhp, maxmp, attackValue, defense,range, shield), combat(this, std::move(skills))
    , team(this, characterTeam), render(this), move(this), buff(this),equip(nullptr){}

Character::Character(const Character& other):Unit(other.name()), bonds(other.bonds), bondCount(other.bondCount),stats(other.stats,this)
    , combat(other.combat,this), team(this, other.team), render(this), move(other.move,this), buff(this),equip(nullptr)
{
    if (other.hasPosition()) {
        setPosition(other.position());
    } else {
        clearPosition();
    }
}

Character::~Character(){
}

void Character::onBattleStart(){
    BattleContext context{this};
    buff.onBattleStart(context);
}


void Character::update(float deltaTime,const Board& board){
    if(buff.isable())buff.update(deltaTime);
    bool ismoving = false;
    if(move.isable())ismoving = move.update(board);
    if(combat.isable())combat.update(deltaTime,ismoving);
    // if(stats.isable())
    //     if(stats.update())return true;//最后再判定死亡
    //return false;

    //死亡交给管理器判断，以防结算顺序导致战斗优势
}

bool Character::isdead(){
    if(stats.isable())
        if(stats.update())return true;
    return false;
}

Bond Character::getBond(int index) const{
    if (index < 0 || index >= bondCount) {
        return Bond::none;
    }
    return bonds[index];
}

bool Character::hasBond(Bond bond) const{
    if (bond == Bond::none) {
        return false;
    }

    for (int i = 0; i < bondCount; ++i) {
        if (bonds[i] == bond) {
            return true;
        }
    }
    return false;
}

Character* Character::gettarget(){
    return move.gettar();
}
