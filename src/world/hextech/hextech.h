#ifndef WORLD_HEXTECH_HEXTECH_H
#define WORLD_HEXTECH_HEXTECH_H

#include <string>
#include <vector>

struct HexTechEffect {
    std::string type;
    int value = 0;
    int buffId = 0;
    int duration = 0;
    std::string modifierKey;
};

struct HexTechDefinition {
    std::string id;
    int layerId = 1;
    std::string name;
    std::string title;
    std::string description;
    std::string cardImagePath;
    std::vector<HexTechEffect> effects;
};

bool isLongTermHexTechEffect(const HexTechEffect& effect);
bool hasLongTermHexTechEffect(const HexTechDefinition& definition);

#endif // WORLD_HEXTECH_HEXTECH_H
