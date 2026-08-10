#include "BlockProperties.h"

#include "content/ContentCatalog.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace
{
constexpr std::size_t count =
    static_cast<std::size_t>(BlockType::TNT) + 1U;

constexpr std::array<BlockProperties, count> properties = {{
    {0.0f, false, false},                                  // Air
    {0.5f, true, false, ToolType::Shovel},                 // Dirt
    {0.6f, true, false, ToolType::Shovel},                 // Grass
    {1.5f, true, false, ToolType::Pickaxe, 0, false},      // Stone
    {2.0f, true, false, ToolType::Pickaxe, 0, false},      // Cobblestone
    {0.6f, true, false, ToolType::Shovel},                 // Gravel
    {-1.0f, false, false},                                 // Water
    {-1.0f, false, false},                                 // Bedrock
    {2.0f, true, false, ToolType::Axe},                    // Oak log
    {0.2f, true, false},                                   // Oak leaves
    {0.5f, true, false, ToolType::Shovel},                 // Sand
    {0.6f, true, false, ToolType::Shovel},                 // Clay
    {3.0f, true, false, ToolType::Pickaxe, 1, false},      // Iron ore
    {3.0f, true, false, ToolType::Pickaxe, 2, false},      // Gold ore
    {3.0f, true, false, ToolType::Pickaxe, 2, false},      // Redstone ore
    {3.0f, true, false, ToolType::Pickaxe, 2, false},      // Diamond ore
    {3.0f, true, false, ToolType::Pickaxe, 0, false},      // Coal ore
    {0.2f, true, false},                                   // Spruce leaves
    {0.2f, true, false},                                   // Birch leaves
    {2.0f, true, false, ToolType::Axe},                    // Spruce log
    {2.0f, true, false, ToolType::Axe},                    // Birch log
    {0.0f, true, true},                                    // Brown mushroom
    {0.0f, true, true},                                    // Red mushroom
    {0.0f, true, true},                                    // Tall grass
    {0.0f, true, true},                                    // Rose
    {0.0f, true, true},                                    // Dandelion
    {2.0f, true, false, ToolType::Pickaxe, 0, false},      // Mossy cobblestone
    {5.0f, true, false, ToolType::Pickaxe, 0, false},      // Spawner
    {2.5f, true, false, ToolType::Axe},                    // Chest
    {1.0f, true, false},                                   // Pumpkin
    {2.5f, true, false, ToolType::Axe},                    // Crafting table
    {2.0f, true, false, ToolType::Axe},                    // Oak planks
    {2.0f, true, false, ToolType::Axe},                    // Spruce planks
    {2.0f, true, false, ToolType::Axe},                    // Birch planks
    {0.8f, true, false, ToolType::Pickaxe, 0, false},      // Sandstone
    {2.0f, true, false, ToolType::Pickaxe, 0, false},      // Bricks
    {0.5f, true, false},                                   // Hay bale
    {0.4f, true, false, ToolType::Axe},                    // Ladder
    {3.0f, true, false, ToolType::Pickaxe, 1, false},      // Lapis block
    {3.0f, true, false, ToolType::Pickaxe, 1, false},      // Lapis ore
    {5.0f, true, false, ToolType::Pickaxe, 1, false},      // Iron block
    {3.0f, true, false, ToolType::Pickaxe, 2, false},      // Gold block
    {0.8f, true, false},                                   // White wool
    {0.8f, true, false},                                   // Orange wool
    {0.8f, true, false},                                   // Magenta wool
    {0.8f, true, false},                                   // Light blue wool
    {0.8f, true, false},                                   // Yellow wool
    {0.8f, true, false},                                   // Lime wool
    {0.8f, true, false},                                   // Pink wool
    {0.8f, true, false},                                   // Gray wool
    {0.8f, true, false},                                   // Light gray wool
    {0.8f, true, false},                                   // Cyan wool
    {0.8f, true, false},                                   // Purple wool
    {0.8f, true, false},                                   // Blue wool
    {0.8f, true, false},                                   // Brown wool
    {0.8f, true, false},                                   // Green wool
    {0.8f, true, false},                                   // Red wool
    {0.8f, true, false},                                   // Black wool
    {10.0f, true, false, ToolType::Pickaxe, 3, false},     // Obsidian
    {3.5f, true, false, ToolType::Pickaxe, 0, false},      // Furnace
    {3.5f, true, false, ToolType::Pickaxe, 0, false},      // Lit furnace
    {-1.0f, false, false},                                 // Lava
    {2.0f, true, false, ToolType::Axe},                    // Jungle log
    {0.2f, true, false},                                   // Jungle leaves
    {2.0f, true, false, ToolType::Axe},                    // Acacia log
    {0.2f, true, false},                                   // Acacia leaves
    {2.0f, true, false, ToolType::Axe},                    // Dark oak log
    {0.2f, true, false},                                   // Dark oak leaves
    {2.0f, true, false, ToolType::Axe},                    // Jungle planks
    {2.0f, true, false, ToolType::Axe},                    // Acacia planks
    {2.0f, true, false, ToolType::Axe},                    // Dark oak planks
    {0.5f, true, false, ToolType::Shovel},                 // Podzol
    {0.6f, true, false, ToolType::Shovel},                 // Mycelium
    {0.2f, true, false, ToolType::Shovel},                 // Snow
    {0.5f, true, false, ToolType::Pickaxe},                // Ice
    {0.4f, true, false},                                   // Cactus
    {0.0f, true, true},                                    // Sugar cane
    {0.6f, true, false, ToolType::Shovel},                 // Farmland
    {0.0f, true, true},                                    // Wheat
    {0.0f, true, true},                                    // Carrots
    {0.0f, true, true},                                    // Potatoes
    {0.0f, true, true},                                    // Beetroots
    {0.3f, true, false},                                   // Glass
    {0.4f, true, false, ToolType::Pickaxe, 0, false},      // Netherrack
    {0.5f, true, false, ToolType::Shovel},                 // Soul sand
    {2.0f, true, false, ToolType::Pickaxe, 0, false},      // Nether bricks
    {0.3f, true, false},                                   // Glowstone
    {3.0f, true, false, ToolType::Pickaxe, 0, false},      // End stone
    {0.0f, true, true},                                    // Redstone wire
    {0.0f, true, true},                                    // Redstone torch
    {0.5f, true, false, ToolType::Pickaxe},                // Lever
    {0.0f, true, true},                                    // Repeater
    {0.5f, true, false, ToolType::Pickaxe},                // Piston
    {0.5f, true, false, ToolType::Pickaxe},                // Sticky piston
    {0.0f, true, true}                                     // TNT
}};
}

const BlockProperties& getBlockProperties(BlockType block) noexcept
{
    if (const mc::content::ContentCatalog* catalog =
            mc::content::ContentCatalog::active())
    {
        if (const mc::content::BlockDefinition* definition =
                catalog->block(block))
        {
            return definition->behaviour.breaking;
        }
    }
    return properties[static_cast<std::size_t>(block)];
}

bool canHarvestBlock(
    BlockType block,
    const ToolProperties& tool) noexcept
{
    const BlockProperties& value = getBlockProperties(block);
    if (!value.breakable || value.hardness < 0.0f)
        return false;

    if (value.harvestableByHand)
        return true;

    return tool.type == value.effectiveTool &&
           tool.harvestLevel >= value.requiredHarvestLevel;
}

float getBlockStrengthPerTick(
    BlockType block,
    const ToolProperties& tool,
    bool onGround,
    bool underwater,
    int hasteAmplifier,
    int fatigueAmplifier,
    bool aquaAffinity) noexcept
{
    const BlockProperties& value = getBlockProperties(block);
    if (!value.breakable || value.hardness < 0.0f)
        return 0.0f;
    if (value.instantBreak || value.hardness == 0.0f)
        return 1.0f;

    float playerStrength = 1.0f;
    if (tool.type == value.effectiveTool)
        playerStrength = tool.miningSpeed;

    if (playerStrength > 1.0f && tool.efficiencyLevel > 0)
    {
        playerStrength += static_cast<float>(
            tool.efficiencyLevel * tool.efficiencyLevel + 1
        );
    }

    if (hasteAmplifier >= 0)
        playerStrength *= 1.0f + 0.2f * static_cast<float>(hasteAmplifier + 1);

    if (fatigueAmplifier >= 0)
    {
        constexpr float fatigueMultipliers[] = {0.3f, 0.09f, 0.0027f, 0.00081f};
        playerStrength *= fatigueMultipliers[std::min(fatigueAmplifier, 3)];
    }

    if (underwater && !aquaAffinity)
        playerStrength /= 5.0f;
    if (!onGround)
        playerStrength /= 5.0f;

    const float divisor = canHarvestBlock(block, tool) ? 30.0f : 100.0f;
    return playerStrength / value.hardness / divisor;
}

float getBlockBreakTimeSeconds(
    BlockType block,
    const ToolProperties& tool,
    bool onGround,
    bool underwater,
    int hasteAmplifier,
    int fatigueAmplifier,
    bool aquaAffinity) noexcept
{
    constexpr float ticksPerSecond = 20.0f;
    const float strength = getBlockStrengthPerTick(
        block,
        tool,
        onGround,
        underwater,
        hasteAmplifier,
        fatigueAmplifier,
        aquaAffinity
    );

    if (strength <= 0.0f)
        return std::numeric_limits<float>::infinity();

    return std::ceil(1.0f / strength) / ticksPerSecond;
}
