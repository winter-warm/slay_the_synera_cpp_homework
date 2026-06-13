#include "hextech.h"

bool isLongTermHexTechEffect(const HexTechEffect& effect) {
    return effect.type != "grant_gold" &&
           effect.type != "choose_basic_equipment" &&
           effect.type != "choose_supply" &&
           effect.type != "choose_advanced_equipment";
}

bool hasLongTermHexTechEffect(const HexTechDefinition& definition) {
    for (const HexTechEffect& effect : definition.effects) {
        if (isLongTermHexTechEffect(effect)) {
            return true;
        }
    }
    return false;
}
