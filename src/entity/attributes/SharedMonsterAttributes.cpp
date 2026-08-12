#include "entity/attributes/SharedMonsterAttributes.h"

namespace mc::entity
{
const IAttribute SharedMonsterAttributes::MAX_HEALTH(
    "generic.maxHealth", 20.0, 0.0, 1024.0, true);
const IAttribute SharedMonsterAttributes::FOLLOW_RANGE(
    "generic.followRange", 32.0, 0.0, 2048.0);
const IAttribute SharedMonsterAttributes::KNOCKBACK_RESISTANCE(
    "generic.knockbackResistance", 0.0, 0.0, 1.0);
const IAttribute SharedMonsterAttributes::MOVEMENT_SPEED(
    "generic.movementSpeed", 0.699999988079071, 0.0, 1024.0, true);
const IAttribute SharedMonsterAttributes::FLYING_SPEED(
    "generic.flyingSpeed", 0.4000000059604645, 0.0, 1024.0, true);
const IAttribute SharedMonsterAttributes::ATTACK_DAMAGE(
    "generic.attackDamage", 2.0, 0.0, 2048.0);
const IAttribute SharedMonsterAttributes::ATTACK_SPEED(
    "generic.attackSpeed", 4.0, 0.0, 1024.0, true);
const IAttribute SharedMonsterAttributes::ARMOR(
    "generic.armor", 0.0, 0.0, 30.0, true);
const IAttribute SharedMonsterAttributes::ARMOR_TOUGHNESS(
    "generic.armorToughness", 0.0, 0.0, 20.0, true);
const IAttribute SharedMonsterAttributes::LUCK(
    "generic.luck", 0.0, -1024.0, 1024.0, true);
}
