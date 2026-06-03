#include "maplayer.h"
#include <algorithm>

const MapNode* MapLayer::nodeById(int nodeId) const {
    for (const MapNode& node : nodes) {
        if (node.id == nodeId) {
            return &node;
        }
    }
    return nullptr;
}

void MapLayer::rebuildRowsFromNodes() {
    int maxRow = -1;
    for (const MapNode& node : nodes) {
        maxRow = std::max(maxRow, node.row);
    }

    rows.clear();
    if (maxRow < 0) {
        return;
    }

    rows.resize(static_cast<size_t>(maxRow + 1));
    for (const MapNode& node : nodes) {
        if (node.row >= 0) {
            rows[static_cast<size_t>(node.row)].push_back(node);
        }
    }

    for (std::vector<MapNode>& row : rows) {
        std::sort(row.begin(), row.end(), [](const MapNode& left, const MapNode& right) {
            return left.position.first < right.position.first;
        });
    }
}

void MapLayer::rebuildNextPointers() {
    for (MapNode& node : nodes) {
        node.next.clear();
        for (int nextId : node.nextNodeIds) {
            if (MapNode* nextNode = nodeById(nextId)) {
                node.next.push_back(nextNode);
            }
        }
    }

    rebuildRowsFromNodes();
    for (std::vector<MapNode>& row : rows) {
        for (MapNode& node : row) {
            node.next.clear();
            for (int nextId : node.nextNodeIds) {
                if (MapNode* nextNode = nodeById(nextId)) {
                    node.next.push_back(nextNode);
                }
            }
        }
    }
}

MapNode* MapLayer::nodeById(int nodeId) {
    for (MapNode& node : nodes) {
        if (node.id == nodeId) {
            return &node;
        }
    }
    return nullptr;
}
