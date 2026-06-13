#ifndef WORLD_HEXTECH_HEXTECHREPOSITORY_H
#define WORLD_HEXTECH_HEXTECHREPOSITORY_H

#include "hextech.h"
#include <QJsonObject>
#include <optional>
#include <string>
#include <vector>

class HexTechRepository {
public:
    HexTechRepository();

    std::vector<HexTechDefinition> allForLayer(int layerId) const;
    std::vector<HexTechDefinition> choicesFor(int seed, int layerId, int nodeId, int count = 3) const;
    std::optional<HexTechDefinition> findById(const std::string& id) const;

private:
    QJsonObject root;
};

#endif // WORLD_HEXTECH_HEXTECHREPOSITORY_H
