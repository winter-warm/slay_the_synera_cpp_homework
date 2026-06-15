#ifndef COMBAT_BATTLEREPOSITORY_H
#define COMBAT_BATTLEREPOSITORY_H

#include "world/event/eventtypes.h"
#include <QJsonObject>
#include <optional>

struct BattleNodeContext {
    int layerId = 1;
    int nodeId = -1;
    int nodeRow = 0;
    int seed = 0;
    BattleKind kind = BattleKind::Normal;
};

class BattleRepository {
public:
    BattleRepository();

    std::optional<BattleConfig> battleFor(const BattleNodeContext& context) const;

private:
    QJsonObject root;
};

#endif // COMBAT_BATTLEREPOSITORY_H
