#pragma once

#include "Item.h"
#include "core/ResourceLocation.h"
#include "entity/EntityUuid.h"

#include <glm/glm.hpp>

namespace mc::entity
{
struct MobPersistentState
{
    core::ResourceLocation type{"minecraft:pig"};
    EntityUuid uuid{};
    EntityUuid ownerUuid{};
    EntityUuid loveCauseUuid{};
    EntityUuid leashHolderUuid{};
    glm::vec3 position{};
    glm::vec3 velocity{};
    float yaw = 0.0f;
    float health = 1.0f;
    int ticksExisted = 0;
    int growingAge = 0;
    int forcedAge = 0;
    int inLove = 0;
    int variant = 0;
    int temper = 0;
    bool tamed = false;
    bool sitting = false;
    bool sheared = false;
    bool saddled = false;
    bool leashed = false;
    ItemStack armor{};
};
}
