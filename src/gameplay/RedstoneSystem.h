#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_set>

class World;

namespace mc::gameplay
{
class RedstoneSystem
{
public:
    void onBlockChanged(int x, int y, int z);
    void tick(World& world, std::size_t updateBudget = 2048);
    [[nodiscard]] std::size_t pendingUpdates() const noexcept;

private:
    struct Position { int x; int y; int z; };
    std::deque<Position> queue_;
    std::unordered_set<std::uint64_t> queued_;

    [[nodiscard]] static std::uint64_t key(int x, int y, int z) noexcept;
    void enqueue(int x, int y, int z);
    [[nodiscard]] int incomingPower(const World& world, int x, int y, int z) const;
};
}
