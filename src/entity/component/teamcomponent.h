#ifndef TEAMCOMPONENT_H
#define TEAMCOMPONENT_H

#include "component.h"

enum class teams:char{pc,enemy};
class TeamComponent : public component
{
private:
    teams team = teams::pc;
public:
    TeamComponent(Character* owner,teams t):component(owner),team(t){}
    TeamComponent(Character* owner,int teamInt):TeamComponent(owner,static_cast<teams>(teamInt)){}
    TeamComponent(Character* owner,const TeamComponent& other):component(other,owner),team(other.team){}

    teams getteam(){return team;}
};

#endif // TEAMCOMPONENT_H
