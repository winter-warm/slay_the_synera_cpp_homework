#ifndef EQUIPMENT_H
#define EQUIPMENT_H
#include<string>
#include<vector>


enum class EquipmentEffectType {
    ModifyStats,
    AddSkill,
    AddBuff
};

struct EquipmentEffect {
    EquipmentEffectType type;

    int statType;
    int value;

    int skillId;
    int buffId;
};

class Equipment {

private:
    int itemId;
    std::string itemName;
    std::vector<EquipmentEffect> itemEffects;
    bool equipped = false;
public:
    int id() const;
    const std::string& name() const;
    const std::vector<EquipmentEffect>& effects() const;

};

#endif // EQUIPMENT_H
