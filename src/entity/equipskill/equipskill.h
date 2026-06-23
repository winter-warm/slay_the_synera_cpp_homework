#ifndef EQUIPSKILL_H
#define EQUIPSKILL_H

#include <memory>

class Character;
class Equipment;
class buff;

namespace equipskill {

std::unique_ptr<buff> createEquipmentBuff(const Equipment& equipment);
void addEquipmentStartBuffs(Character* character, const Equipment& equipment);
void addEquipmentActiveSkills(Character* character, const Equipment& equipment);

}

#endif // EQUIPSKILL_H
