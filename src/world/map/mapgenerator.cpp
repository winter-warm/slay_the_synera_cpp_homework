#include "mapgenerator.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

static constexpr double mapWidth = 1280.0;
static constexpr double mapHeight = 2200.0;
static constexpr double usableWidthRatio = 0.75;
static constexpr double topY = 70.0;
static constexpr double bottomY = mapHeight - 70.0;
static constexpr int slotCount = 6;
static constexpr int lotteryEventId = 400;
static constexpr int recruitEventId = 401;
static constexpr int horseMarketEventId = 402;
static constexpr int chessElderEventId = 403;

struct NodeWeights {
    int normalBattle = 0;
    int eliteBattle = 0;
    int event = 0;
    int rest = 0;
};

static int randomInt(std::mt19937& rng, int minValue, int maxValue) {
    std::uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(rng);
}

static double randomReal(std::mt19937& rng, double minValue, double maxValue) {
    std::uniform_real_distribution<double> dist(minValue, maxValue);
    return dist(rng);
}

static MapNodeType typeForEventId(int eventId) {
    if (eventId == MapEventId::Rest || eventId == MapEventId::Start) {
        return MapNodeType::Rest;
    }
    if (eventId == MapEventId::EliteBattle || eventId == MapEventId::Boss) {
        return MapNodeType::EliteBattle;
    }
    if (eventId >= MapEventId::FirstEvent) {
        return MapNodeType::Event;
    }
    return MapNodeType::NormalBattle;
}

static int randomFromPool(std::mt19937& rng, const std::vector<int>& pool) {
    return pool[static_cast<size_t>(randomInt(rng, 0, static_cast<int>(pool.size()) - 1))];
}

static int randomLayerEventId(std::mt19937& rng, int layerId) {
    static const std::vector<int> layer1Events = {101, 102, 103, 104, 105, 106,
                                                  107, 108, 109, 110, 111, 112};
    static const std::vector<int> layer2Events = {201, 202, 203, 204, 205, 206, 207, 208, 209, 210};
    static const std::vector<int> layer3Events = {301, 302, 303, 304, 305, 306, 307, 308};

    if (layerId == 1) {
        return randomFromPool(rng, layer1Events);
    }
    if (layerId == 2) {
        return randomFromPool(rng, layer2Events);
    }
    return randomFromPool(rng, layer3Events);
}

static NodeWeights weightsForLayer(int layerId) {
    if (layerId == 2) {
        return {6, 2, 4, 3};
    }
    if (layerId == 3) {
        return {6, 5, 2, 2};
    }
    return {25, 8, 42, 25};
}

static int randomEventId(std::mt19937& rng, int layerId) {
    const NodeWeights weights = weightsForLayer(layerId);
    const int total = weights.normalBattle + weights.eliteBattle + weights.event + weights.rest;
    const int roll = randomInt(rng, 1, total);
    if (roll <= weights.rest) {
        return MapEventId::Rest;
    }
    if (roll <= weights.rest + weights.event) {
        return randomLayerEventId(rng, layerId);
    }
    if (roll <= weights.rest + weights.event + weights.normalBattle) {
        return MapEventId::NormalBattle;
    }
    return MapEventId::EliteBattle;
}

static std::vector<int> makeRowCounts(std::mt19937& rng, int middleRows) {
    for (int attempt = 0; attempt < 200; ++attempt) {
        std::vector<int> counts(static_cast<size_t>(middleRows), 0);
        for (int i = 0; i < middleRows; ++i) {
            const int roll = randomInt(rng, 1, 100);
            counts[static_cast<size_t>(i)] = roll <= 35 ? 3 : (roll <= 82 ? 4 : 5);
        }
        counts.front() = std::min(counts.front(), 3);
        counts.back() = std::min(counts.back(), 3);

        bool valid = std::abs(counts.front() - 1) <= 2 && std::abs(counts.back() - 1) <= 2;
        for (int i = 1; valid && i < middleRows; ++i) {
            valid = std::abs(counts[static_cast<size_t>(i)] - counts[static_cast<size_t>(i - 1)]) <= 2;
        }
        if (valid) {
            return counts;
        }
    }

    std::vector<int> fallback(static_cast<size_t>(middleRows), 4);
    fallback.front() = 3;
    fallback.back() = 3;
    return fallback;
}

static std::vector<int> chooseSlots(std::mt19937& rng, int count) {
    std::vector<int> chosenSlots;
    if (count <= 1) {
        chosenSlots = {randomInt(rng, 2, 3)};
    } else if (count == 2) {
        chosenSlots = randomInt(rng, 0, 1) == 0 ? std::vector<int>{1, 4} : std::vector<int>{0, 5};
    } else if (count == 3) {
        chosenSlots = randomInt(rng, 0, 1) == 0 ? std::vector<int>{0, 3, 5} : std::vector<int>{1, 2, 4};
    } else if (count == 4) {
        chosenSlots = {0, 2, 3, 5};
    } else {
        chosenSlots = {0, 1, 2, 4, 5};
    }
    return chosenSlots;
}

static std::pair<double, double> positionForNode(std::mt19937& rng, int row, int rowCount,
                                                  int slot, bool centered) {
    const double usableWidth = mapWidth * usableWidthRatio;
    const double usableLeft = (mapWidth - usableWidth) * 0.5;
    const double rowGap = (bottomY - topY) / static_cast<double>(rowCount - 1);
    double x = mapWidth * 0.5;
    double y = bottomY - rowGap * row;

    if (!centered) {
        const double slotWidth = usableWidth / slotCount;
        x = usableLeft + slotWidth * (slot + 0.5);
        x += randomReal(rng, -18.0, 18.0);
        y += randomReal(rng, -16.0, 16.0);
    }

    return {x, y};
}

static MapNode makeNode(int id, int layerId, int row, int lane, int eventId,
                        MapNodeRole role, std::pair<double, double> position, bool fixed = false) {
    MapNode node;
    node.id = id;
    node.event_id = eventId;
    node.layerId = layerId;
    node.row = row;
    node.lane = lane;
    node.position = position;
    node.type = typeForEventId(eventId);
    node.role = role;
    node.fixed = fixed || role != MapNodeRole::Normal;
    return node;
}

static int fixedEventIdForRow(int layerId, int row, int totalRows) {
    const int lastPlayableRow = totalRows - 2;
    if (layerId == 1) {
        if (row == 1 || row == lastPlayableRow) {
            return MapEventId::Rest;
        }
        if (row == 6) {
            return lotteryEventId;
        }
        if (row == 8) {
            return chessElderEventId;
        }
        if (row == lastPlayableRow - 2) {
            return recruitEventId;
        }
        if (row == lastPlayableRow - 1) {
            return horseMarketEventId;
        }
        return -1;
    }

    if (row == 1 || row == lastPlayableRow) {
        return MapEventId::Rest;
    }
    return -1;
}

static int fixedRowNodeCount(int row, int totalRows, int previousRowCount) {
    const int lastPlayableRow = totalRows - 2;
    if (row == 1) {
        return 3;
    }
    if (row == lastPlayableRow) {
        return 1;
    }
    return previousRowCount;
}

static void addPlayableRows(MapLayer& layer, std::mt19937& rng, int layerId, int middleRows,
                            int totalRows, const std::vector<int>& rowCounts, int& nextId) {
    for (int i = 0; i < middleRows; ++i) {
        const int row = i + 1;
        const int fixedEventId = fixedEventIdForRow(layerId, row, totalRows);
        const bool fixedRow = fixedEventId >= 0;
        const int previousRowCount = static_cast<int>(layer.rows[static_cast<size_t>(row - 1)].size());
        const int nodeCount = fixedRow ? fixedRowNodeCount(row, totalRows, previousRowCount)
                                       : rowCounts[static_cast<size_t>(i)];
        const bool centeredFixedRest = fixedRow && row == totalRows - 2;
        const std::vector<int> chosenSlots = centeredFixedRest ? std::vector<int>{2} : chooseSlots(rng, nodeCount);
        for (int laneIndex = 0; laneIndex < static_cast<int>(chosenSlots.size()); ++laneIndex) {
            const int slot = chosenSlots[static_cast<size_t>(laneIndex)];
            layer.rows[static_cast<size_t>(row)].push_back(
                makeNode(nextId++, layerId, row, slot,
                         fixedRow ? fixedEventId : randomEventId(rng, layerId),
                         MapNodeRole::Normal,
                         positionForNode(rng, row, totalRows, slot, centeredFixedRest),
                         fixedRow));
        }
    }
}

static int connectionCount(std::mt19937& rng, int maxCount) {
    const int roll = randomInt(rng, 1, 100);
    int count = roll <= 48 ? 1 : (roll <= 83 ? 2 : 3);
    return std::min(count, maxCount);
}

static void addNext(MapNode& from, const MapNode& to) {
    if (std::find(from.nextNodeIds.begin(), from.nextNodeIds.end(), to.id) == from.nextNodeIds.end()) {
        from.nextNodeIds.push_back(to.id);
    }
}

static void connectNearestRows(std::vector<MapNode>& fromRow, const std::vector<MapNode>& toRow,
                               std::mt19937& rng) {
    if (fromRow.empty() || toRow.empty()) {
        return;
    }

    std::vector<MapNode*> fromByX;
    std::vector<const MapNode*> toByX;
    for (MapNode& node : fromRow) {
        fromByX.push_back(&node);
    }
    for (const MapNode& node : toRow) {
        toByX.push_back(&node);
    }
    std::sort(fromByX.begin(), fromByX.end(), [](const MapNode* left, const MapNode* right) {
        return left->position.first < right->position.first;
    });
    std::sort(toByX.begin(), toByX.end(), [](const MapNode* left, const MapNode* right) {
        return left->position.first < right->position.first;
    });

    const int fromCount = static_cast<int>(fromByX.size());
    const int toCount = static_cast<int>(toByX.size());

    auto targetForSource = [fromCount, toCount](int sourceIndex) {
        if (fromCount <= 0) {
            return 0;
        }
        return std::clamp(static_cast<int>(std::floor(sourceIndex * toCount / static_cast<double>(fromCount))),
                          0, toCount - 1);
    };

    auto sourceForTarget = [fromCount, toCount](int targetIndex) {
        if (toCount <= 0) {
            return 0;
        }
        return std::clamp(static_cast<int>(std::floor(targetIndex * fromCount / static_cast<double>(toCount))),
                          0, fromCount - 1);
    };

    std::vector<std::vector<int>> targetsBySource(static_cast<size_t>(fromCount));
    for (int j = 0; j < toCount; ++j) {
        targetsBySource[static_cast<size_t>(sourceForTarget(j))].push_back(j);
    }

    for (int i = 0; i < fromCount; ++i) {
        std::vector<int>& targetIndexes = targetsBySource[static_cast<size_t>(i)];
        const int baseTarget = targetForSource(i);
        if (std::find(targetIndexes.begin(), targetIndexes.end(), baseTarget) == targetIndexes.end()) {
            targetIndexes.push_back(baseTarget);
            std::sort(targetIndexes.begin(), targetIndexes.end());
        }

        const int maxEdges = std::min(connectionCount(rng, static_cast<int>(targetIndexes.size())),
                                      static_cast<int>(targetIndexes.size()));
        for (int edgeIndex = 0; edgeIndex < maxEdges; ++edgeIndex) {
            addNext(*fromByX[static_cast<size_t>(i)],
                    *toByX[static_cast<size_t>(targetIndexes[static_cast<size_t>(edgeIndex)])]);
        }
    }

    for (int j = 0; j < toCount; ++j) {
        const MapNode& to = *toByX[static_cast<size_t>(j)];
        bool hasIncoming = false;
        for (const MapNode* from : fromByX) {
            hasIncoming = std::find(from->nextNodeIds.begin(), from->nextNodeIds.end(), to.id) != from->nextNodeIds.end();
            if (hasIncoming) {
                break;
            }
        }
        if (hasIncoming) {
            continue;
        }

        addNext(*fromByX[static_cast<size_t>(sourceForTarget(j))], to);
    }
}

static void connectRows(MapLayer& layer, std::mt19937& rng) {
    if (layer.rows.size() < 2) {
        return;
    }

    const int lastRow = static_cast<int>(layer.rows.size()) - 1;
    for (int row = 0; row < lastRow - 1; ++row) {
        connectNearestRows(layer.rows[static_cast<size_t>(row)],
                           layer.rows[static_cast<size_t>(row + 1)], rng);
    }

    for (MapNode& node : layer.rows[static_cast<size_t>(lastRow - 1)]) {
        addNext(node, layer.rows[static_cast<size_t>(lastRow)][0]);
    }
}

static void flattenRows(MapLayer& layer) {
    layer.nodes.clear();
    for (const std::vector<MapNode>& row : layer.rows) {
        for (const MapNode& node : row) {
            layer.nodes.push_back(node);
        }
    }
    layer.rebuildNextPointers();
}

MapLayer MapGenerator::makeLayer(int layerId) const {
    MapLayer layer;
    layer.layerId = layerId;
    layer.backgroundPath = "assets/maps/" + std::to_string(layerId) + ".png";
    return layer;
}

MapGraph MapGenerator::generate(int seed) const {
    MapGraph graph;
    graph.layers.reserve(3);

    std::mt19937 rng1(static_cast<unsigned int>(seed * 2654435761u + 101u));
    MapLayer layer1 = makeLayer(1);
    {
        const int middleRows = randomInt(rng1, 13, 15);
        const int totalRows = middleRows + 2;
        const std::vector<int> rowCounts = makeRowCounts(rng1, middleRows);
        layer1.rows.resize(static_cast<size_t>(totalRows));
        int nextId = 1000;
        layer1.startNodeId = nextId;
        layer1.rows[0].push_back(makeNode(nextId++, 1, 0, 2, MapEventId::Start,
                                          MapNodeRole::Start, positionForNode(rng1, 0, totalRows, 2, true)));
        addPlayableRows(layer1, rng1, 1, middleRows, totalRows, rowCounts, nextId);
        layer1.endNodeId = nextId;
        layer1.rows[static_cast<size_t>(totalRows - 1)].push_back(
            makeNode(nextId++, 1, totalRows - 1, 2, MapEventId::Boss,
                     MapNodeRole::End, positionForNode(rng1, totalRows - 1, totalRows, 2, true)));
        connectRows(layer1, rng1);
        flattenRows(layer1);
    }
    graph.layers.push_back(layer1);

    std::mt19937 rng2(static_cast<unsigned int>(seed * 2654435761u + 202u));
    MapLayer layer2 = makeLayer(2);
    {
        const int middleRows = randomInt(rng2, 13, 15);
        const int totalRows = middleRows + 2;
        const std::vector<int> rowCounts = makeRowCounts(rng2, middleRows);
        layer2.rows.resize(static_cast<size_t>(totalRows));
        int nextId = 2000;
        layer2.startNodeId = nextId;
        layer2.rows[0].push_back(makeNode(nextId++, 2, 0, 2, MapEventId::Start,
                                          MapNodeRole::Start, positionForNode(rng2, 0, totalRows, 2, true)));
        addPlayableRows(layer2, rng2, 2, middleRows, totalRows, rowCounts, nextId);
        layer2.endNodeId = nextId;
        layer2.rows[static_cast<size_t>(totalRows - 1)].push_back(
            makeNode(nextId++, 2, totalRows - 1, 2, MapEventId::Boss,
                     MapNodeRole::End, positionForNode(rng2, totalRows - 1, totalRows, 2, true)));
        connectRows(layer2, rng2);
        flattenRows(layer2);
    }
    graph.layers.push_back(layer2);

    std::mt19937 rng3(static_cast<unsigned int>(seed * 2654435761u + 303u));
    MapLayer layer3 = makeLayer(3);
    {
        const int middleRows = randomInt(rng3, 13, 15);
        const int totalRows = middleRows + 2;
        const std::vector<int> rowCounts = makeRowCounts(rng3, middleRows);
        layer3.rows.resize(static_cast<size_t>(totalRows));
        int nextId = 3000;
        layer3.startNodeId = nextId;
        layer3.rows[0].push_back(makeNode(nextId++, 3, 0, 2, MapEventId::Start,
                                          MapNodeRole::Start, positionForNode(rng3, 0, totalRows, 2, true)));
        addPlayableRows(layer3, rng3, 3, middleRows, totalRows, rowCounts, nextId);
        layer3.endNodeId = nextId;
        layer3.rows[static_cast<size_t>(totalRows - 1)].push_back(
            makeNode(nextId++, 3, totalRows - 1, 2, MapEventId::Boss,
                     MapNodeRole::End, positionForNode(rng3, totalRows - 1, totalRows, 2, true)));
        connectRows(layer3, rng3);
        flattenRows(layer3);
    }
    graph.layers.push_back(layer3);

    for (MapLayer& layer : graph.layers) {
        layer.rebuildNextPointers();
    }

    return graph;
}
