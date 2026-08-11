#include "Block.h"
#include "content/BlockState.h"
#include "entity/ai/Goal.h"
#include "entity/navigation/PathNavigation.h"
#include "game/GameBootstrap.h"
#include "gameplay/GameplayRegistries.h"

#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>

namespace
{
class TestContext final : public mc::entity::ai::GoalContext {};

class CountingGoal final : public mc::entity::ai::Goal
{
public:
    explicit CountingGoal(bool& enabled, bool interruptible = true)
        : enabled_(&enabled), interruptible_(interruptible)
    {
        setMutexBits(1);
    }

    bool shouldExecute(mc::entity::ai::GoalContext&) override
    {
        return *enabled_;
    }
    bool shouldContinue(mc::entity::ai::GoalContext&) override
    {
        return *enabled_;
    }
    void start(mc::entity::ai::GoalContext&) override { ++starts; }
    void reset(mc::entity::ai::GoalContext&) override { ++resets; }
    void tick(mc::entity::ai::GoalContext&) override { ++ticks; }
    bool interruptible() const noexcept override { return interruptible_; }

    int starts = 0;
    int resets = 0;
    int ticks = 0;

private:
    bool* enabled_ = nullptr;
    bool interruptible_ = true;
};

class GridWorld final : public mc::entity::navigation::NavigationBlockAccess
{
public:
    GridWorld()
    {
        blocks.fill(BlockType::Air);
        for (int x = 0; x < Width; ++x)
        for (int z = 0; z < Depth; ++z)
            set(x, 0, z, BlockType::Stone);
    }

    void set(int x, int y, int z, BlockType block)
    {
        blocks[index(x, y, z)] = block;
    }

    mc::content::BlockState blockState(
        int x, int y, int z) const override
    {
        return mc::content::BlockState(blocks[index(x, y, z)]);
    }

    bool loaded(int x, int y, int z) const override
    {
        return x >= 0 && x < Width && y >= 0 && y < Height &&
               z >= 0 && z < Depth;
    }

private:
    static constexpr int Width = 12;
    static constexpr int Height = 6;
    static constexpr int Depth = 12;
    std::array<BlockType, Width * Height * Depth> blocks{};

    static std::size_t index(int x, int y, int z)
    {
        assert(x >= 0 && x < Width && y >= 0 && y < Height &&
               z >= 0 && z < Depth);
        return static_cast<std::size_t>(
            x + Width * (z + Depth * y)
        );
    }
};

bool hasItem(const std::vector<ItemType>& items, ItemType item)
{
    return std::find(items.begin(), items.end(), item) != items.end();
}
}

int main()
{
    TestContext context;
    bool highEnabled = false;
    bool lowEnabled = true;
    auto high = std::make_unique<CountingGoal>(highEnabled);
    auto low = std::make_unique<CountingGoal>(lowEnabled);
    CountingGoal* highView = high.get();
    CountingGoal* lowView = low.get();
    mc::entity::ai::GoalSelector selector;
    selector.add(1, std::move(high));
    selector.add(5, std::move(low));
    selector.tick(context);
    assert(lowView->starts == 1 && lowView->ticks == 1);
    selector.tick(context);
    selector.tick(context);
    assert(lowView->ticks == 3);
    highEnabled = true;
    selector.tick(context);
    assert(highView->starts == 1 && highView->ticks == 1);
    assert(lowView->resets == 1);
    assert(selector.runningCount() == 1U);
    selector.disableControlFlag(1);
    selector.tick(context);
    selector.tick(context);
    selector.tick(context);
    assert(highView->resets == 1);
    assert(selector.runningCount() == 0U);

    GridWorld world;
    for (int z = 0; z <= 2; ++z)
    {
        world.set(3, 1, z, BlockType::Stone);
        world.set(3, 2, z, BlockType::Stone);
    }
    mc::entity::navigation::NavigationSettings settings;
    settings.width = 0.6f;
    settings.height = 1.8f;
    mc::entity::navigation::PathFinder finder;
    const auto path = finder.findPath(
        world, settings, {1.5f, 1.0f, 1.5f}, {6.5f, 1.0f, 1.5f}, 24.0f
    );
    assert(path && path->size() > 2U);
    assert(path->finalPoint() != nullptr);
    assert(path->finalPoint()->position == glm::ivec3(6, 1, 1));
    for (std::size_t index = 0; index < path->size(); ++index)
        assert(path->at(index).position != glm::ivec3(3, 1, 1));

    mc::game::GameBootstrap bootstrap("assets");
    bootstrap.loadContentModules();
    bootstrap.freezeRegistries();
    const auto& mobs = bootstrap.gameplay().mobs();
    const auto* cow = mobs.find(mc::core::ResourceLocation("minecraft:cow"));
    const auto* wolf = mobs.find(mc::core::ResourceLocation("minecraft:wolf"));
    const auto* ocelot = mobs.find(mc::core::ResourceLocation("minecraft:ocelot"));
    const auto* parrot = mobs.find(mc::core::ResourceLocation("minecraft:parrot"));
    const auto* horse = mobs.find(mc::core::ResourceLocation("minecraft:horse"));
    const auto* donkey = mobs.find(mc::core::ResourceLocation("minecraft:donkey"));
    const auto* mule = mobs.find(mc::core::ResourceLocation("minecraft:mule"));
    assert(cow && cow->ageable && cow->breedable);
    assert(hasItem(cow->breedingItems, ItemType::WheatItem));
    assert(wolf && wolf->tameableKind == mc::gameplay::TameableKind::Wolf);
    assert(hasItem(wolf->tamingItems, ItemType::Bone));
    assert(hasItem(wolf->breedingItems, ItemType::CookedRabbit));
    assert(ocelot && ocelot->tameableKind ==
           mc::gameplay::TameableKind::Ocelot);
    assert(parrot && parrot->tamingItems.size() == 4U);
    assert(horse && horse->maximumTemper == 100 &&
           std::abs(horse->stepHeight - 1.0f) < 0.001f);
    assert(donkey && donkey->tameableKind ==
           mc::gameplay::TameableKind::Donkey);
    assert(mule && mule->tameableKind ==
           mc::gameplay::TameableKind::Mule);
}
