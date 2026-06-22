#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <optional>
#include <string>
#include <vector>

enum class EquipmentGroup {
    Mold = 1,
    Element = 2,
    Advanced = 3
};

enum class EquipmentStat {
    AttackPercent,
    AttackSpeedPercent,
    AttackRangeFlat,
    CritChancePercent,
    CritDamagePercent,
    InitialMpFlat,
    MaxMpFlat,
    MaxHpPercent,
    InitialShieldMaxHpPercent,
    DefensePercent,
    DodgeChancePercent,
    HealingDonePercent,
    HealingTakenPercent
};

struct EquipmentModifier {
    EquipmentStat stat = EquipmentStat::AttackPercent;
    int value = 0;
};

struct EquipmentRecipe {
    int moldId = 0;
    int elementId = 0;
};

class Equipment {

public:
    Equipment() = default;
    Equipment(int id,
              EquipmentGroup group,
              std::string name,
              std::string title,
              std::string description,
              std::string iconPath,
              std::string effectText,
              std::vector<EquipmentModifier> modifiers,
              std::vector<int> buffIds,
              std::string skillName,
              std::string skillText,
              std::optional<EquipmentRecipe> recipe = std::nullopt);

    int id() const;
    EquipmentGroup group() const;
    const std::string& name() const;
    const std::string& title() const;
    const std::string& description() const;
    const std::string& iconPath() const;
    const std::string& effectText() const;
    const std::vector<EquipmentModifier>& modifiers() const;
    const std::vector<int>& buffIds() const;
    const std::string& skillName() const;
    const std::string& skillText() const;
    const std::optional<EquipmentRecipe>& recipe() const;
    bool isBasic() const;
    bool isAdvanced() const;

private:
    int itemId = 0;
    EquipmentGroup itemGroup = EquipmentGroup::Mold;
    std::string itemName;
    std::string itemTitle;
    std::string itemDescription;
    std::string itemIconPath;
    std::string itemEffectText;
    std::vector<EquipmentModifier> itemModifiers;
    std::vector<int> itemBuffIds;
    std::string itemSkillName;
    std::string itemSkillText;
    std::optional<EquipmentRecipe> itemRecipe;
};

#endif // EQUIPMENT_H
