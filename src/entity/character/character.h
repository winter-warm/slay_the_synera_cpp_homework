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
class Board;
enum class Bond:int{none,soldier,mage,animal,building,pastor,witch,beast,chess,assassin};
class Character : public Unit
{
private:
    int templateIdValue = 0;
    int rarityValue = 1;
    std::string displayNameValue;
    std::string magicNameValue;
    std::string skillDescriptionValue;
    std::array<Bond, 2> bonds;
    int bondCount;//1 或 2
    StatsComponent stats;
    AttackComponent combat;
    TeamComponent team;
    RenderComponent render;
    MoveComponent move;
    BuffComponent buff;
    Equipment* equip = nullptr;
    Board* currentBoard = nullptr;
    int untargetableLocks = 0;
public:
    explicit Character(std::string name,const std::array<Bond, 2>& bonds,int bondCount,std::vector<Skill> skills,
                       int maxhp = 100,int maxmp = 3,int attack = 10,int defense = 0,int range = 1,int shield = 0,
                       teams team = teams::pc,int templateId = 0,int rarity = 1,
                       std::string displayName = {},std::string magicName = {},std::string skillDescription = {});
    Character(const Character& other);
    ~Character();
    void onBattleStart();
    void update(float deltaTime, Board& board) override;
    int getBondCount() const{return bondCount;}
    Bond getBond(int index) const;
    bool hasBond(Bond bond) const;
    int templateId() const{return templateIdValue;}
    int rarity() const{return rarityValue;}
    const std::string& displayName() const{return displayNameValue;}
    const std::string& magicName() const{return magicNameValue;}
    const std::string& skillDescription() const{return skillDescriptionValue;}
    StatsComponent& getstats(){return stats;}
    RenderComponent& getrender(){return render;}
    BuffComponent& getbuff(){return buff;}
    MoveComponent& getmove(){return move;}
    Equipment* getquipe(){return equip;}
    const Equipment* getquipe() const{return equip;}
    TeamComponent& getteam(){return team;}
    AttackComponent& getattack(){return combat;}
    Character* gettarget();
    Board* getCurrentBoard() const{return currentBoard;}
    void setCurrentBoard(Board* board){currentBoard = board;}
    bool isTargetable() const{return untargetableLocks <= 0;}
    void addUntargetableLock(){++untargetableLocks;}
    void removeUntargetableLock(){if(untargetableLocks > 0){--untargetableLocks;}}
    bool isdead();
};

#endif // CHARACTER_H
