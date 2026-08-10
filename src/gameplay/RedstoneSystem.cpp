#include "gameplay/RedstoneSystem.h"

#include "World.h"

#include <algorithm>
#include <array>

namespace mc::gameplay
{
namespace
{
constexpr std::array<std::array<int, 3>, 6> Neighbours{{
    {{-1, 0, 0}}, {{1, 0, 0}}, {{0, -1, 0}},
    {{0, 1, 0}}, {{0, 0, -1}}, {{0, 0, 1}}
}};
}

std::uint64_t RedstoneSystem::key(int x, int y, int z) noexcept
{
    const std::uint64_t px = static_cast<std::uint32_t>(x) & 0x3FFFFFFULL;
    const std::uint64_t pz = static_cast<std::uint32_t>(z) & 0x3FFFFFFULL;
    return (px << 38U) | (pz << 12U) | static_cast<std::uint64_t>(y & 0xFFF);
}

void RedstoneSystem::enqueue(int x, int y, int z)
{
    if (y < 0 || y >= Chunk::HEIGHT)
        return;
    if (queued_.insert(key(x, y, z)).second)
        queue_.push_back({x, y, z});
}

void RedstoneSystem::onBlockChanged(int x, int y, int z)
{
    enqueue(x, y, z);
    for (const auto& offset : Neighbours)
        enqueue(x + offset[0], y + offset[1], z + offset[2]);
}

int RedstoneSystem::incomingPower(
    const World& world, int x, int y, int z) const
{
    int power = 0;
    for (const auto& offset : Neighbours)
    {
        const auto state = world.getBlockState(
            x + offset[0], y + offset[1], z + offset[2]
        );
        if (state.block() == BlockType::Lever && (state.properties() & 1U) != 0U)
            power = 15;
        else if (state.block() == BlockType::RedstoneTorch &&
                 (state.properties() & 1U) != 0U)
            power = 15;
        else if (state.block() == BlockType::Repeater &&
                 (state.properties() & 1U) != 0U)
            power = 15;
        else if (state.block() == BlockType::RedstoneWire)
            power = std::max(power, std::max(0, static_cast<int>(state.properties()) - 1));
    }
    return power;
}

void RedstoneSystem::tick(World& world, std::size_t updateBudget)
{
    std::size_t processed = 0;
    while (!queue_.empty() && processed < updateBudget)
    {
        const Position position = queue_.front();
        queue_.pop_front();
        queued_.erase(key(position.x, position.y, position.z));
        ++processed;

        const auto state = world.getBlockState(position.x, position.y, position.z);
        if (state.block() == BlockType::RedstoneWire)
        {
            const int power = incomingPower(
                world, position.x, position.y, position.z
            );
            if (power != state.properties())
            {
                world.setBlockAndMetadata(
                    position.x, position.y, position.z,
                    BlockType::RedstoneWire,
                    static_cast<std::uint8_t>(power)
                );
            }
        }
        else if (state.block() == BlockType::Repeater)
        {
            const std::uint8_t powered = incomingPower(
                world, position.x, position.y, position.z
            ) > 0 ? 1U : 0U;
            if ((state.properties() & 1U) != powered)
            {
                world.setBlockAndMetadata(
                    position.x, position.y, position.z,
                    BlockType::Repeater, powered
                );
            }
        }
    }
}

std::size_t RedstoneSystem::pendingUpdates() const noexcept
{
    return queue_.size();
}
}
