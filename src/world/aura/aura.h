#ifndef WORLD_AURA_AURA_H
#define WORLD_AURA_AURA_H

#include "world/hextech/hextech.h"
#include <string>
#include <vector>

struct ActiveAura {
    std::string hexTechId;
    std::vector<HexTechEffect> effects;
};

#endif // WORLD_AURA_AURA_H
