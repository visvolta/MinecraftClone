#pragma once

#include "entity/attributes/IAttribute.h"

namespace mc::entity
{
// net.minecraft.entity.SharedMonsterAttributes
struct SharedMonsterAttributes
{
    static const IAttribute MAX_HEALTH;
    static const IAttribute FOLLOW_RANGE;
    static const IAttribute KNOCKBACK_RESISTANCE;
    static const IAttribute MOVEMENT_SPEED;
    static const IAttribute FLYING_SPEED;
    static const IAttribute ATTACK_DAMAGE;
    static const IAttribute ATTACK_SPEED;
    static const IAttribute ARMOR;
    static const IAttribute ARMOR_TOUGHNESS;
    static const IAttribute LUCK;
};
}
