#ifndef WORLD_MAP_MAPLAYER_H
#define WORLD_MAP_MAPLAYER_H

#include "mapnode.h"
#include <string>
#include <unordered_map>
#include <vector>

struct LayerConfig {
    int layerId = 1;
    std::string backgroundPath;
    int minNodeCount = 12;
    int maxNodeCount = 15;
    std::unordered_map<MapNodeType, int> typeWeights;
    std::vector<MapNode> fixedNodes;
};

class MapLayer {
public:
    int layerId = 1;
    std::string backgroundPath;
    std::vector<std::vector<MapNode>> rows;
    std::vector<MapNode> nodes;
    int startNodeId = -1;
    int endNodeId = -1;

    const MapNode* nodeById(int nodeId) const;
    MapNode* nodeById(int nodeId);
    void rebuildRowsFromNodes();
    void rebuildNextPointers();
};

#endif // WORLD_MAP_MAPLAYER_H
