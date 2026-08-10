#pragma once

#include "Block.h"
#include "Item.h"

#include <cstdint>
#include <random>
#include <vector>

[[nodiscard]] std::vector<ItemStack> getBlockDrops(
    BlockType block,
    std::uint8_t metadata,
    const ToolProperties& tool,
    std::mt19937& random
);
