#include "equipment.h"

int Equipment::id() const{
    return itemId;
}

const std::string& Equipment::name() const{
    return itemName;
}

const std::vector<EquipmentEffect>& Equipment::effects() const{
    return itemEffects;
}
