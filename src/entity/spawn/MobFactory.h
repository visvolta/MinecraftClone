#pragma once

#include "core/ResourceLocation.h"

#include <memory>

class World;

namespace mc::entity
{
class Mob;

[[nodiscard]] std::unique_ptr<Mob> createMob(
    const core::ResourceLocation& type,
    World& world
);
}
