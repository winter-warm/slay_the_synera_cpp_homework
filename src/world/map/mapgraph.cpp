#include "mapgraph.h"
#include <QJsonArray>
#include <QString>

std::string mapNodeTypeToString(MapNodeType type) {
    switch (type) {
    case MapNodeType::Rest:
        return "rest";
    case MapNodeType::Event:
        return "event";
    case MapNodeType::NormalBattle:
        return "normalBattle";
    case MapNodeType::EliteBattle:
        return "eliteBattle";
    }
    return "normalBattle";
}

MapNodeType mapNodeTypeFromString(const std::string& value) {
    if (value == "rest") {
        return MapNodeType::Rest;
    }
    if (value == "event") {
        return MapNodeType::Event;
    }
    if (value == "eliteBattle") {
        return MapNodeType::EliteBattle;
    }
    return MapNodeType::NormalBattle;
}

std::string mapNodeRoleToString(MapNodeRole role) {
    switch (role) {
    case MapNodeRole::Start:
        return "start";
    case MapNodeRole::End:
        return "end";
    case MapNodeRole::Normal:
        return "normal";
    }
    return "normal";
}

MapNodeRole mapNodeRoleFromString(const std::string& value) {
    if (value == "start") {
        return MapNodeRole::Start;
    }
    if (value == "end") {
        return MapNodeRole::End;
    }
    return MapNodeRole::Normal;
}

const MapLayer* MapGraph::layerById(int layerId) const {
    for (const MapLayer& layer : layers) {
        if (layer.layerId == layerId) {
            return &layer;
        }
    }
    return nullptr;
}

MapLayer* MapGraph::layerById(int layerId) {
    for (MapLayer& layer : layers) {
        if (layer.layerId == layerId) {
            return &layer;
        }
    }
    return nullptr;
}

QJsonObject MapGraph::toJson() const {
    QJsonArray layerArray;
    for (const MapLayer& layer : layers) {
        QJsonObject layerObject;
        layerObject["layerId"] = layer.layerId;
        layerObject["backgroundPath"] = QString::fromStdString(layer.backgroundPath);
        layerObject["startNodeId"] = layer.startNodeId;
        layerObject["endNodeId"] = layer.endNodeId;

        QJsonArray nodeArray;
        for (const MapNode& node : layer.nodes) {
            QJsonObject nodeObject;
            nodeObject["id"] = node.id;
            nodeObject["event_id"] = node.event_id;
            nodeObject["layerId"] = node.layerId;
            nodeObject["row"] = node.row;
            nodeObject["lane"] = node.lane;
            nodeObject["x"] = node.position.first;
            nodeObject["y"] = node.position.second;
            nodeObject["type"] = QString::fromStdString(mapNodeTypeToString(node.type));
            nodeObject["role"] = QString::fromStdString(mapNodeRoleToString(node.role));
            nodeObject["fixed"] = node.fixed;

            QJsonArray nextArray;
            for (int nextId : node.nextNodeIds) {
                nextArray.append(nextId);
            }
            nodeObject["nextNodeIds"] = nextArray;
            nodeArray.append(nodeObject);
        }

        layerObject["nodes"] = nodeArray;
        layerArray.append(layerObject);
    }

    QJsonObject object;
    object["layers"] = layerArray;
    return object;
}

MapGraph MapGraph::fromJson(const QJsonObject& object) {
    MapGraph graph;
    const QJsonArray layerArray = object.value("layers").toArray();
    for (const QJsonValue& layerValue : layerArray) {
        const QJsonObject layerObject = layerValue.toObject();
        MapLayer layer;
        layer.layerId = layerObject.value("layerId").toInt(1);
        layer.backgroundPath = layerObject.value("backgroundPath").toString().toStdString();
        layer.startNodeId = layerObject.value("startNodeId").toInt(-1);
        layer.endNodeId = layerObject.value("endNodeId").toInt(-1);

        const QJsonArray nodeArray = layerObject.value("nodes").toArray();
        for (const QJsonValue& nodeValue : nodeArray) {
            const QJsonObject nodeObject = nodeValue.toObject();
            MapNode node;
            node.id = nodeObject.value("id").toInt(-1);
            node.event_id = nodeObject.value("event_id").toInt(MapEventId::NormalBattle);
            node.layerId = nodeObject.value("layerId").toInt(layer.layerId);
            node.row = nodeObject.value("row").toInt();
            node.lane = nodeObject.value("lane").toInt();
            node.position = {nodeObject.value("x").toDouble(),
                             nodeObject.value("y").toDouble()};
            node.type = mapNodeTypeFromString(nodeObject.value("type").toString().toStdString());
            node.role = mapNodeRoleFromString(nodeObject.value("role").toString().toStdString());
            node.fixed = nodeObject.value("fixed").toBool(false);

            const QJsonArray nextArray = nodeObject.value("nextNodeIds").toArray();
            for (const QJsonValue& nextValue : nextArray) {
                node.nextNodeIds.push_back(nextValue.toInt());
            }
            layer.nodes.push_back(node);
        }

        layer.rebuildNextPointers();
        graph.layers.push_back(layer);
    }
    return graph;
}
