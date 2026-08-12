#pragma once

#include "content/BlockState.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <queue>
#include <unordered_set>
#include <vector>

class World;

namespace mc::gameplay
{
class RedstoneSystem
{
public:
    void onBlockChanged(int x, int y, int z);
    void tick(World& world, std::size_t updateBudget = 4096);
    [[nodiscard]] std::size_t pendingUpdates() const noexcept;
    [[nodiscard]] std::uint64_t gameTick() const noexcept;

private:
    struct Position
    {
        int x = 0;
        int y = 0;
        int z = 0;

        [[nodiscard]] bool operator==(const Position&) const noexcept = default;
    };

    struct ScheduledUpdate
    {
        std::uint64_t dueTick = 0;
        Position position{};
        mc::content::BlockState expected{};

        [[nodiscard]] bool operator>(const ScheduledUpdate& other) const noexcept
        {
            return dueTick > other.dueTick;
        }
    };

    std::deque<Position> queue_;
    std::unordered_set<std::uint64_t> queued_;
    std::priority_queue<
        ScheduledUpdate,
        std::vector<ScheduledUpdate>,
        std::greater<>
    > scheduled_;
    std::uint64_t gameTick_ = 0;

    [[nodiscard]] static std::uint64_t key(int x, int y, int z) noexcept;
    void enqueue(int x, int y, int z);
    void enqueueNeighbours(int x, int y, int z);
    void schedule(
        int x, int y, int z,
        mc::content::BlockState expected,
        std::uint64_t delay
    );

    [[nodiscard]] int directPower(
        const World& world,
        int x, int y, int z,
        int fromX, int fromY, int fromZ
    ) const;
    [[nodiscard]] int incomingPower(
        const World& world,
        int x, int y, int z
    ) const;
    [[nodiscard]] int wireNeighbourPower(
        const World& world,
        int x, int y, int z
    ) const;

    void updatePosition(World& world, const Position& position);
};
}
