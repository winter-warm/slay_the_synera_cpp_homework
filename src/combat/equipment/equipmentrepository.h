#ifndef EQUIPMENTREPOSITORY_H
#define EQUIPMENTREPOSITORY_H

#include "equipment.h"

#include <optional>
#include <vector>

class EquipmentRepository {
public:
    EquipmentRepository();

    const std::vector<Equipment>& allBasic() const;
    const std::vector<Equipment>& allAdvanced() const;
    std::optional<Equipment> findBasic(EquipmentGroup group, int id) const;
    std::optional<Equipment> findAdvanced(int id) const;
    bool canCompose(const Equipment& left, const Equipment& right) const;
    std::optional<Equipment> composeResult(const Equipment& left, const Equipment& right) const;

private:
    std::vector<Equipment> basicEquipment;
    std::vector<Equipment> advancedEquipment;
};

#endif // EQUIPMENTREPOSITORY_H
