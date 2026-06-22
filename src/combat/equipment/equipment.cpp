#include "equipment.h"

#include <utility>

Equipment::Equipment(int id,
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
                     std::optional<EquipmentRecipe> recipe)
    : itemId(id),
      itemGroup(group),
      itemName(std::move(name)),
      itemTitle(std::move(title)),
      itemDescription(std::move(description)),
      itemIconPath(std::move(iconPath)),
      itemEffectText(std::move(effectText)),
      itemModifiers(std::move(modifiers)),
      itemBuffIds(std::move(buffIds)),
      itemSkillName(std::move(skillName)),
      itemSkillText(std::move(skillText)),
      itemRecipe(std::move(recipe)) {}

int Equipment::id() const{
    return itemId;
}

EquipmentGroup Equipment::group() const{
    return itemGroup;
}

const std::string& Equipment::name() const{
    return itemName;
}

const std::string& Equipment::title() const{
    return itemTitle;
}

const std::string& Equipment::description() const{
    return itemDescription;
}

const std::string& Equipment::iconPath() const{
    return itemIconPath;
}

const std::string& Equipment::effectText() const{
    return itemEffectText;
}

const std::vector<EquipmentModifier>& Equipment::modifiers() const{
    return itemModifiers;
}

const std::vector<int>& Equipment::buffIds() const{
    return itemBuffIds;
}

const std::string& Equipment::skillName() const{
    return itemSkillName;
}

const std::string& Equipment::skillText() const{
    return itemSkillText;
}

const std::optional<EquipmentRecipe>& Equipment::recipe() const{
    return itemRecipe;
}

bool Equipment::isBasic() const{
    return itemGroup == EquipmentGroup::Mold || itemGroup == EquipmentGroup::Element;
}

bool Equipment::isAdvanced() const{
    return itemGroup == EquipmentGroup::Advanced;
}
