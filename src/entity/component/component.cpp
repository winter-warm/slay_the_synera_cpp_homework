#include "component.h"
component::component(const component& other,Character* owner):owner(owner),enabled(other.enabled){}
