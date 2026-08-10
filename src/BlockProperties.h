#pragma once
#include "Block.h"
#include "ToolProperties.h"

struct BlockProperties
{
    float hardness = 0.0f;
    bool breakable = true;
    bool instantBreak = false;
    ToolType effectiveTool = ToolType::None;
    int requiredHarvestLevel = -1;
    bool harvestableByHand = true;
};

[[nodiscard]] const BlockProperties& getBlockProperties(BlockType block) noexcept;
[[nodiscard]] bool canHarvestBlock(
    BlockType block,
    const ToolProperties& tool) noexcept;
[[nodiscard]] float getBlockStrengthPerTick(
    BlockType block,
    const ToolProperties& tool,
    bool onGround = true,
    bool underwater = false,
    int hasteAmplifier = -1,
    int fatigueAmplifier = -1,
    bool aquaAffinity = false) noexcept;
[[nodiscard]] float getBlockBreakTimeSeconds(
    BlockType block,
    const ToolProperties& tool,
    bool onGround = true,
    bool underwater = false,
    int hasteAmplifier = -1,
    int fatigueAmplifier = -1,
    bool aquaAffinity = false) noexcept;
