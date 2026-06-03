#ifndef WORLD_MAP_MAPGENERATOR_H
#define WORLD_MAP_MAPGENERATOR_H

#include "mapgraph.h"
#include <vector>

class MapGenerator {
public:
    MapGraph generate(int seed) const;

private:
    MapLayer makeLayer(int layerId) const;
};

#endif // WORLD_MAP_MAPGENERATOR_H
