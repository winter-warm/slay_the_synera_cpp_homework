#ifndef WORLD_BAG_BAG_H
#define WORLD_BAG_BAG_H

#include <string>
#include <vector>

struct HexTechCardRecord {
    std::string hexTechId;
    int layerId = 1;
    std::string title;
    std::string description;
    std::string cardImagePath;
    int pickedAtNodeId = -1;
};

struct OwnedCharacterCard {
    int templateId = 0;
    int starLevel = 1;
};

struct ShopOffer {
    int templateId = 0;
    bool sold = false;
};

struct ShopState {
    int level = 1;
    int exp = 0;
    int rollCounter = 0;
    std::vector<ShopOffer> offers;
};

#endif // WORLD_BAG_BAG_H
