#ifndef COMPONENT_H
#define COMPONENT_H

class Character;
class component
{
protected:
    Character* owner;
    bool enabled = true;
public:
    virtual ~component() = default;
    component(Character* owner):owner(owner){}
    component(const component& other,Character* owner);
    virtual void Enable()  { enabled = true; }//virtual是为了obstacle可以把这个函数删掉(写空)，保证不会打开
    virtual void Disable() { enabled = false; }
    bool isable(){return enabled; }
//    virtual void update(float deltaTime,Character* target) = 0;//buff attack用到deltatime，buff move用到target
//放弃抽象类了，参数差的太多
};

#endif // COMPONENT_H
