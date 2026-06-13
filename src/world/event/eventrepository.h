#ifndef WORLD_EVENT_EVENTREPOSITORY_H
#define WORLD_EVENT_EVENTREPOSITORY_H

#include "eventtypes.h"
#include <QJsonObject>
#include <optional>

class EventRepository {
public:
    EventRepository();

    std::optional<BattleConfig> battleFor(const EventContext& context, BattleKind kind) const;
    std::optional<EventDefinition> eventFor(const EventContext& context,
                                            const std::string& stepId = "start") const;

private:
    QJsonObject root;
};

#endif // WORLD_EVENT_EVENTREPOSITORY_H
