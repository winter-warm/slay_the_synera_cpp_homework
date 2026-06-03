#ifndef WORLD_MAP_MAPGRAPH_H
#define WORLD_MAP_MAPGRAPH_H

#include "maplayer.h"
#include <QJsonObject>
#include <string>
#include <vector>

class MapGraph {
public:
    std::vector<MapLayer> layers;

    const MapLayer* layerById(int layerId) const;
    MapLayer* layerById(int layerId);

    QJsonObject toJson() const;
    static MapGraph fromJson(const QJsonObject& object);
};

std::string mapNodeTypeToString(MapNodeType type);
MapNodeType mapNodeTypeFromString(const std::string& value);
std::string mapNodeRoleToString(MapNodeRole role);
MapNodeRole mapNodeRoleFromString(const std::string& value);

#endif // WORLD_MAP_MAPGRAPH_H
