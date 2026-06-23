#ifndef STATSCOMPONENT_H
#define STATSCOMPONENT_H
#include "component.h"
class StatsComponent:public component
{
private:
    int hp;
    int maxhp;
    int defense;
    int mp=0;
    int maxmp;
    int attack;
    int shield;
    int range;
public:
    StatsComponent(Character* owner,int maxhp,int maxmp,int attack,int defense,int range = 1,int shield = 0):
        component(owner),maxhp(maxhp),hp(maxhp),maxmp(maxmp),attack(attack),defense(defense),range(range),shield(shield){}
    StatsComponent(const StatsComponent& other,Character* owner);
    ~StatsComponent()=default;

    void modifyHP(int delta){hp+=delta;}
    void modifyMAXHP(int delta);
    bool modifyMP(int delta);
    void modifyMAXMP(int delta);
    void modifyATTACL(int delta){attack+=delta;if(attack<=0)attack=0;}
    void modifyDEFESE(int delta){defense+=delta;if(defense<=0)defense=0;}
    void modifySHIELF(int delta){shield+=delta;if(shield<=0)shield = 0;}
    void modifyRANGE(int delta){range+=delta;if(range<1)range = 1;}
    void setHP(int value);
    void setATTACK(int value){attack = value;if(attack<=0)attack=0;}
    void beattacked(int damage, Character* attacker = nullptr);
    void heal(int amount, Character* source = nullptr);

    int getHP(){return hp;}
    int getMAXHP(){return maxhp;}
    int getDEFENSE(){return defense;}
    int getMAXMP(){return maxmp;}
    int getMP(){return mp;}
    int getATTACK(){return attack;}
    int getSHIELD(){return shield;}
    int getRANGE(){return range;}

    bool update();//死亡传yes
};

#endif // STATSCOMPONENT_H
