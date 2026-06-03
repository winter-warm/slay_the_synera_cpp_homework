#ifndef CHARACTER_H
#define CHARACTER_H

#include "entity/component/statscomponent.h"
#include "entity/component/attackcomponent.h"
#include "entity/component/movecomponent.h"
#include "entity/component/teamcomponent.h"
#include "entity/component/rendercomponent.h"
#include "entity/component/buffcomponent.h"
#include "entity/unit.h"

#include <array>
#include <vector>
//buff已包含memory

class Equipment;
enum class Bond:int{none,soldier,mage,animal,building,pastor,witch,beast,chess};
class Character : public Unit
{
private:
    std::array<Bond, 2> bonds;
    int bondCount;//1 或 2
    StatsComponent stats;
    AttackComponent combat;
    TeamComponent team;
    RenderComponent render;
    MoveComponent move;
    BuffComponent buff;
    Equipment* equip = nullptr;
public:
    explicit Character(std::string name,const std::array<Bond, 2>& bonds,int bondCount,std::vector<Skill> skills,
                       int maxhp = 100,int maxmp = 3,int attack = 10,int defense = 0,int range = 1,int shield = 0,
                       teams team = teams::pc);
    Character(const Character& other);
    ~Character();
    void onBattleStart();
    void update(float deltaTime, const Board& board) override;
    int getBondCount() const{return bondCount;}
    Bond getBond(int index) const;
    bool hasBond(Bond bond) const;
    StatsComponent& getstats(){return stats;}
    RenderComponent& getrender(){return render;}
    BuffComponent& getbuff(){return buff;}
    MoveComponent& getmove(){return move;}
    Equipment* getquipe(){return equip;}
    TeamComponent& getteam(){return team;}
    AttackComponent& getattack(){return combat;}
    Character* gettarget();
    bool isdead();
};

#endif // CHARACTER_H
