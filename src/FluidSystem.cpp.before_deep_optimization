#include "FluidSystem.h"

#include "FluidState.h"
#include "World.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
constexpr std::array<glm::ivec3, 4> HorizontalDirections{{
    {-1, 0, 0},
    {1, 0, 0},
    {0, 0, -1},
    {0, 0, 1}
}};

constexpr std::array<int, 4> OppositeDirection{{1, 0, 3, 2}};
constexpr int NoFlowCost = 1000;
}

FluidSystem::FluidSystem(World& world, std::uint32_t seed)
    : world_(world),
      random_(seed ^ 0x6c8e9cf5U)
{
}

std::size_t FluidSystem::PositionHash::operator()(
    const Position& position) const noexcept
{
    std::size_t seed = std::hash<int>{}(position.x);
    seed ^= std::hash<int>{}(position.y) + 0x9e3779b9U +
            (seed << 6U) + (seed >> 2U);
    seed ^= std::hash<int>{}(position.z) + 0x9e3779b9U +
            (seed << 6U) + (seed >> 2U);
    return seed;
}

bool FluidSystem::LaterTick::operator()(
    const ScheduledTick& left,
    const ScheduledTick& right) const noexcept
{
    return left.dueTick > right.dueTick;
}

void FluidSystem::tick()
{
    ++worldTick_;
    constexpr int maximumUpdatesPerTick = 4096;
    int processed = 0;

    while (!ticks_.empty() &&
           ticks_.top().dueTick <= worldTick_ &&
           processed < maximumUpdatesPerTick)
    {
        const ScheduledTick scheduledTick = ticks_.top();
        ticks_.pop();

        const auto current = scheduled_.find(scheduledTick.position);
        if (current == scheduled_.end() ||
            current->second.dueTick != scheduledTick.dueTick ||
            current->second.liquid != scheduledTick.liquid)
        {
            continue;
        }

        scheduled_.erase(current);
        if (!world_.isBlockLoaded(
                scheduledTick.position.x,
                scheduledTick.position.y,
                scheduledTick.position.z))
        {
            schedule(
                scheduledTick.position.x,
                scheduledTick.position.y,
                scheduledTick.position.z,
                scheduledTick.liquid
            );
            ++processed;
            continue;
        }
        if (world_.getBlock(
                scheduledTick.position.x,
                scheduledTick.position.y,
                scheduledTick.position.z) == scheduledTick.liquid)
        {
            updateFluid(scheduledTick.position, scheduledTick.liquid);
        }
        ++processed;
    }
}

void FluidSystem::onBlockChanged(
    int x,
    int y,
    int z,
    BlockType,
    BlockType newBlock)
{
    if (newBlock == BlockType::Lava && hardenLava(x, y, z))
        return;

    constexpr std::array<glm::ivec3, 6> neighbours{{
        {-1, 0, 0}, {1, 0, 0}, {0, -1, 0},
        {0, 1, 0}, {0, 0, -1}, {0, 0, 1}
    }};

    if (isLiquid(newBlock))
        schedule(x, y, z, newBlock);

    for (const glm::ivec3& offset : neighbours)
    {
        const int neighbourX = x + offset.x;
        const int neighbourY = y + offset.y;
        const int neighbourZ = z + offset.z;
        const BlockType neighbour = world_.getBlock(
            neighbourX,
            neighbourY,
            neighbourZ
        );

        if (neighbour == BlockType::Lava &&
            hardenLava(neighbourX, neighbourY, neighbourZ))
        {
            continue;
        }

        if (isLiquid(neighbour))
            schedule(neighbourX, neighbourY, neighbourZ, neighbour);
    }
}

void FluidSystem::schedule(
    int x,
    int y,
    int z,
    BlockType liquid)
{
    if (!isLiquid(liquid))
        return;

    const Position position{x, y, z};
    const std::uint64_t dueTick =
        worldTick_ + static_cast<std::uint64_t>(
            FluidState::tickRate(liquid)
        );
    const auto existing = scheduled_.find(position);
    if (existing != scheduled_.end() &&
        existing->second.liquid == liquid &&
        existing->second.dueTick <= dueTick)
    {
        return;
    }

    scheduled_[position] = {dueTick, liquid};
    ticks_.push({dueTick, position, liquid});
}

std::vector<FluidScheduledTickSnapshot>
FluidSystem::snapshotScheduledTicks() const
{
    std::vector<FluidScheduledTickSnapshot> result;
    result.reserve(scheduled_.size());
    for (const auto& [position, pending] : scheduled_)
    {
        result.push_back({
            position.x,
            position.y,
            position.z,
            pending.liquid,
            pending.dueTick > worldTick_ ? pending.dueTick - worldTick_ : 1
        });
    }
    return result;
}

void FluidSystem::restoreScheduledTicks(
    const std::vector<FluidScheduledTickSnapshot>& restored)
{
    ticks_ = {};
    scheduled_.clear();
    for (const FluidScheduledTickSnapshot& entry : restored)
    {
        if (!isLiquid(entry.liquid))
            continue;
        const Position position{entry.x, entry.y, entry.z};
        const std::uint64_t dueTick = worldTick_ +
            std::max<std::uint64_t>(1, entry.remainingTicks);
        scheduled_[position] = {dueTick, entry.liquid};
        ticks_.push({dueTick, position, entry.liquid});
    }
}

void FluidSystem::updateFluid(
    const Position& position,
    BlockType liquid)
{
    const int x = position.x;
    const int y = position.y;
    const int z = position.z;

    if (liquid == BlockType::Lava && hardenLava(x, y, z))
        return;

    int decay = flowDecay(x, y, z, liquid);
    const int spreadStep = FluidState::spreadStep(liquid);
    bool settle = true;

    if (decay > 0)
    {
        int adjacentSources = 0;
        int smallest = -100;
        smallest = smallestFlowDecay(
            x - 1, y, z, liquid, smallest, adjacentSources);
        smallest = smallestFlowDecay(
            x + 1, y, z, liquid, smallest, adjacentSources);
        smallest = smallestFlowDecay(
            x, y, z - 1, liquid, smallest, adjacentSources);
        smallest = smallestFlowDecay(
            x, y, z + 1, liquid, smallest, adjacentSources);

        int nextDecay = smallest + spreadStep;
        if (nextDecay >= FluidState::FallingFlag || smallest < 0)
            nextDecay = -1;

        const int aboveDecay = flowDecay(x, y + 1, z, liquid);
        if (aboveDecay >= 0)
            nextDecay = aboveDecay >= FluidState::FallingFlag
                ? aboveDecay
                : aboveDecay + FluidState::FallingFlag;

        if (liquid == BlockType::Water && adjacentSources >= 2)
        {
            const BlockType below = world_.getBlock(x, y - 1, z);
            if (isSolid(below) ||
                (below == liquid &&
                 world_.getBlockMetadata(x, y - 1, z) == 0))
            {
                nextDecay = 0;
            }
        }

        if (liquid == BlockType::Lava &&
            decay < FluidState::FallingFlag &&
            nextDecay < FluidState::FallingFlag &&
            nextDecay > decay)
        {
            std::uniform_int_distribution<int> holdDistribution(0, 3);
            if (holdDistribution(random_) != 0)
            {
                nextDecay = decay;
                settle = false;
            }
        }

        if (nextDecay != decay)
        {
            decay = nextDecay;
            if (nextDecay < 0)
            {
                world_.setBlockAndMetadata(
                    x, y, z, BlockType::Air, 0
                );
                return;
            }

            world_.setBlockAndMetadata(
                x,
                y,
                z,
                liquid,
                static_cast<std::uint8_t>(nextDecay)
            );
        }
        else if (!settle)
        {
            schedule(x, y, z, liquid);
        }
    }

    if (canDisplace(x, y - 1, z, liquid))
    {
        const int downwardLevel = decay >= FluidState::FallingFlag
            ? decay
            : decay + FluidState::FallingFlag;
        flowInto(x, y - 1, z, liquid, downwardLevel);
        return;
    }

    if (decay < 0 ||
        (decay != 0 && !blockBlocksFlow(x, y - 1, z)))
    {
        return;
    }

    const std::array<bool, 4> directions =
        optimalFlowDirections(x, y, z, liquid);
    int horizontalLevel = decay + spreadStep;
    if (decay >= FluidState::FallingFlag)
        horizontalLevel = 1;
    if (horizontalLevel >= FluidState::FallingFlag)
        return;

    for (std::size_t direction = 0;
         direction < HorizontalDirections.size();
         ++direction)
    {
        if (!directions[direction])
            continue;
        const glm::ivec3 offset = HorizontalDirections[direction];
        flowInto(
            x + offset.x,
            y,
            z + offset.z,
            liquid,
            horizontalLevel
        );
    }
}

void FluidSystem::flowInto(
    int x,
    int y,
    int z,
    BlockType liquid,
    int level)
{
    if (!canDisplace(x, y, z, liquid))
        return;

    // BlockFlowing turns lava entering water into stone. Performing this
    // before placement preserves the contact instead of overwriting water
    // and then failing to detect it during hardening.
    if (liquid == BlockType::Lava &&
        world_.getBlock(x, y, z) == BlockType::Water)
    {
        world_.setBlockAndMetadata(x, y, z, BlockType::Stone, 0);
        return;
    }

    world_.setBlockAndMetadata(
        x,
        y,
        z,
        liquid,
        FluidState::clampLevel(level)
    );
}

int FluidSystem::flowDecay(
    int x,
    int y,
    int z,
    BlockType liquid) const
{
    if (world_.getBlock(x, y, z) != liquid)
        return -1;
    return static_cast<int>(world_.getBlockMetadata(x, y, z));
}

int FluidSystem::effectiveFlowDecay(
    int x,
    int y,
    int z,
    BlockType liquid) const
{
    int decay = flowDecay(x, y, z, liquid);
    if (decay >= 0)
        decay = FluidState::effectiveLevel(
            static_cast<std::uint8_t>(decay)
        );
    return decay;
}

int FluidSystem::smallestFlowDecay(
    int x,
    int y,
    int z,
    BlockType liquid,
    int currentSmallest,
    int& adjacentSources) const
{
    int decay = flowDecay(x, y, z, liquid);
    if (decay < 0)
        return currentSmallest;
    if (decay == 0)
        ++adjacentSources;
    decay = FluidState::effectiveLevel(
        static_cast<std::uint8_t>(decay)
    );
    return currentSmallest >= 0 && decay >= currentSmallest
        ? currentSmallest
        : decay;
}

bool FluidSystem::blockBlocksFlow(int x, int y, int z) const
{
    if (!world_.isBlockLoaded(x, y, z))
        return true;

    const BlockType block = world_.getBlock(x, y, z);
    if (block == BlockType::Air || isPlant(block) || isLiquid(block))
        return false;
    if (isLadder(block))
        return true;
    return isSolid(block);
}

bool FluidSystem::canDisplace(
    int x,
    int y,
    int z,
    BlockType liquid) const
{
    if (!world_.isBlockLoaded(x, y, z))
        return false;

    const BlockType target = world_.getBlock(x, y, z);
    if (target == liquid || target == BlockType::Lava)
        return false;
    return !blockBlocksFlow(x, y, z);
}

int FluidSystem::calculateFlowCost(
    int x,
    int y,
    int z,
    BlockType liquid,
    int cost,
    int previousDirection) const
{
    int bestCost = NoFlowCost;

    for (std::size_t direction = 0;
         direction < HorizontalDirections.size();
         ++direction)
    {
        if (static_cast<int>(direction) ==
            OppositeDirection[static_cast<std::size_t>(previousDirection)])
        {
            continue;
        }

        const glm::ivec3 offset = HorizontalDirections[direction];
        const int nextX = x + offset.x;
        const int nextZ = z + offset.z;
        if (blockBlocksFlow(nextX, y, nextZ))
            continue;
        if (world_.getBlock(nextX, y, nextZ) == liquid &&
            world_.getBlockMetadata(nextX, y, nextZ) == 0)
        {
            continue;
        }

        if (!blockBlocksFlow(nextX, y - 1, nextZ))
            return cost;

        if (cost < 4)
        {
            bestCost = std::min(
                bestCost,
                calculateFlowCost(
                    nextX,
                    y,
                    nextZ,
                    liquid,
                    cost + 1,
                    static_cast<int>(direction)
                )
            );
        }
    }

    return bestCost;
}

std::array<bool, 4> FluidSystem::optimalFlowDirections(
    int x,
    int y,
    int z,
    BlockType liquid) const
{
    std::array<int, 4> costs{};
    costs.fill(NoFlowCost);

    for (std::size_t direction = 0;
         direction < HorizontalDirections.size();
         ++direction)
    {
        const glm::ivec3 offset = HorizontalDirections[direction];
        const int nextX = x + offset.x;
        const int nextZ = z + offset.z;
        if (blockBlocksFlow(nextX, y, nextZ))
            continue;
        if (world_.getBlock(nextX, y, nextZ) == liquid &&
            world_.getBlockMetadata(nextX, y, nextZ) == 0)
        {
            continue;
        }

        costs[direction] = !blockBlocksFlow(nextX, y - 1, nextZ)
            ? 0
            : calculateFlowCost(
                nextX,
                y,
                nextZ,
                liquid,
                1,
                static_cast<int>(direction)
            );
    }

    const int best = *std::min_element(costs.begin(), costs.end());
    std::array<bool, 4> result{};
    if (best == NoFlowCost)
        return result;
    for (std::size_t direction = 0; direction < costs.size(); ++direction)
        result[direction] = costs[direction] == best;
    return result;
}

bool FluidSystem::hardenLava(int x, int y, int z)
{
    if (world_.getBlock(x, y, z) != BlockType::Lava)
        return false;

    constexpr std::array<glm::ivec3, 5> waterChecks{{
        {-1, 0, 0}, {1, 0, 0}, {0, 0, -1},
        {0, 0, 1}, {0, 1, 0}
    }};
    const bool touchesWater = std::ranges::any_of(
        waterChecks,
        [this, x, y, z](const glm::ivec3& offset)
        {
            return world_.getBlock(
                x + offset.x,
                y + offset.y,
                z + offset.z
            ) == BlockType::Water;
        }
    );
    if (!touchesWater)
        return false;

    const int level = world_.getBlockMetadata(x, y, z);
    if (level == 0)
    {
        world_.setBlockAndMetadata(
            x, y, z, BlockType::Obsidian, 0
        );
        return true;
    }
    if (level <= 4)
    {
        world_.setBlockAndMetadata(
            x, y, z, BlockType::Cobblestone, 0
        );
        return true;
    }
    return false;
}

glm::vec3 FluidSystem::getFlowVector(
    int x,
    int y,
    int z,
    BlockType liquid) const
{
    if (world_.getBlock(x, y, z) != liquid)
        return glm::vec3(0.0f);

    glm::vec3 flow(0.0f);
    const int currentDecay = effectiveFlowDecay(x, y, z, liquid);
    for (const glm::ivec3& offset : HorizontalDirections)
    {
        const int neighbourX = x + offset.x;
        const int neighbourZ = z + offset.z;
        int neighbourDecay = effectiveFlowDecay(
            neighbourX,
            y,
            neighbourZ,
            liquid
        );
        if (neighbourDecay < 0)
        {
            if (!blockBlocksFlow(neighbourX, y, neighbourZ))
            {
                neighbourDecay = effectiveFlowDecay(
                    neighbourX,
                    y - 1,
                    neighbourZ,
                    liquid
                );
                if (neighbourDecay >= 0)
                {
                    const int difference =
                        neighbourDecay - (currentDecay - 8);
                    flow += glm::vec3(offset) *
                        static_cast<float>(difference);
                }
            }
        }
        else
        {
            const int difference = neighbourDecay - currentDecay;
            flow += glm::vec3(offset) * static_cast<float>(difference);
        }
    }

    const int metadata = world_.getBlockMetadata(x, y, z);
    if (FluidState::isFalling(
            static_cast<std::uint8_t>(metadata)))
    {
        bool nextToWall = false;
        for (const glm::ivec3& offset : HorizontalDirections)
        {
            nextToWall = nextToWall ||
                blockBlocksFlow(x + offset.x, y, z + offset.z) ||
                blockBlocksFlow(x + offset.x, y + 1, z + offset.z);
        }
        if (nextToWall)
        {
            const float length = glm::length(flow);
            if (length > 0.00001f)
                flow /= length;
            flow.y -= 6.0f;
        }
    }

    const float length = glm::length(flow);
    return length > 0.00001f ? flow / length : glm::vec3(0.0f);
}

std::size_t FluidSystem::getPendingTickCount() const noexcept
{
    return scheduled_.size();
}
