#ifndef TEAMCOMPONENT_H
#define TEAMCOMPONENT_H

#include "component.h"

#include <optional>

enum class teams:char{pc,enemy};
class TeamComponent : public component
{
private:
    teams baseTeam = teams::pc;
    std::optional<teams> temporaryTeam;
public:
    TeamComponent(Character* owner,teams t):component(owner),baseTeam(t){}
    TeamComponent(Character* owner,int teamInt):TeamComponent(owner,static_cast<teams>(teamInt)){}
    TeamComponent(Character* owner,const TeamComponent& other):component(other,owner),baseTeam(other.baseTeam),temporaryTeam(other.temporaryTeam){}

    teams getteam() const{return temporaryTeam.value_or(baseTeam);}
    teams getBaseTeam() const{return baseTeam;}
    void setTemporaryTeam(teams team){temporaryTeam = team;}
    void clearTemporaryTeam(){temporaryTeam.reset();}
};

#endif // TEAMCOMPONENT_H
