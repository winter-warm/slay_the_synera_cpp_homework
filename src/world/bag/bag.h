#ifndef WORLD_BAG_BAG_H
#define WORLD_BAG_BAG_H

#include <string>

struct HexTechCardRecord {
    std::string hexTechId;
    int layerId = 1;
    std::string title;
    std::string description;
    std::string cardImagePath;
    int pickedAtNodeId = -1;
};

#endif // WORLD_BAG_BAG_H
