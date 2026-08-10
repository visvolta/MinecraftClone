#pragma once

#include "Block.h"

#include <glm/vec3.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <random>
#include <unordered_map>
#include <vector>

class World;

struct FluidScheduledTickSnapshot
{
    int x = 0;
    int y = 0;
    int z = 0;
    BlockType liquid = BlockType::Water;
    std::uint64_t remainingTicks = 0;
};

class FluidSystem
{
public:
    FluidSystem(World& world, std::uint32_t seed);

    void tick();
    void onBlockChanged(
        int x,
        int y,
        int z,
        BlockType oldBlock,
        BlockType newBlock
    );

    [[nodiscard]] glm::vec3 getFlowVector(
        int x,
        int y,
        int z,
        BlockType liquid
    ) const;
    [[nodiscard]] std::size_t getPendingTickCount() const noexcept;
    [[nodiscard]] std::vector<FluidScheduledTickSnapshot>
        snapshotScheduledTicks() const;
    void restoreScheduledTicks(
        const std::vector<FluidScheduledTickSnapshot>& ticks
    );

private:
    struct Position
    {
        int x = 0;
        int y = 0;
        int z = 0;

        bool operator==(const Position&) const = default;
    };

    struct PositionHash
    {
        std::size_t operator()(const Position& position) const noexcept;
    };

    struct ScheduledTick
    {
        std::uint64_t dueTick = 0;
        Position position;
        BlockType liquid = BlockType::Water;
    };

    struct LaterTick
    {
        bool operator()(
            const ScheduledTick& left,
            const ScheduledTick& right
        ) const noexcept;
    };

    struct PendingTick
    {
        std::uint64_t dueTick = 0;
        BlockType liquid = BlockType::Water;
    };

    World& world_;
    std::uint64_t worldTick_ = 0;
    std::priority_queue<
        ScheduledTick,
        std::vector<ScheduledTick>,
        LaterTick
    > ticks_;
    std::unordered_map<Position, PendingTick, PositionHash> scheduled_;
    std::mt19937 random_;

    void schedule(int x, int y, int z, BlockType liquid);
    void updateFluid(const Position& position, BlockType liquid);
    void flowInto(
        int x,
        int y,
        int z,
        BlockType liquid,
        int level
    );

    [[nodiscard]] int flowDecay(
        int x,
        int y,
        int z,
        BlockType liquid
    ) const;
    [[nodiscard]] int effectiveFlowDecay(
        int x,
        int y,
        int z,
        BlockType liquid
    ) const;
    [[nodiscard]] int smallestFlowDecay(
        int x,
        int y,
        int z,
        BlockType liquid,
        int currentSmallest,
        int& adjacentSources
    ) const;
    [[nodiscard]] bool blockBlocksFlow(int x, int y, int z) const;
    [[nodiscard]] bool canDisplace(
        int x,
        int y,
        int z,
        BlockType liquid
    ) const;
    [[nodiscard]] int calculateFlowCost(
        int x,
        int y,
        int z,
        BlockType liquid,
        int cost,
        int previousDirection
    ) const;
    [[nodiscard]] std::array<bool, 4> optimalFlowDirections(
        int x,
        int y,
        int z,
        BlockType liquid
    ) const;
    bool hardenLava(int x, int y, int z);

};
