#include "BlockBreakingController.h"

#include "BlockProperties.h"
#include "ItemEntityManager.h"
#include "World.h"

#include <algorithm>
#include <vector>

void BlockBreakingController::update(
    bool attackHeld,
    const RaycastHit& hit,
    float deltaTime,
    ItemStack& heldStack,
    bool playerOnGround,
    bool playerUnderwater,
    int hasteAmplifier,
    int fatigueAmplifier,
    bool aquaAffinity,
    World& world,
    ItemEntityManager& itemEntities)
{
    // Beta gameplay runs at exactly 20 logical ticks per second.
    constexpr double tickSeconds = 1.0 / 20.0;
    constexpr int maximumCatchUpTicks = 2;
    const ToolProperties tool = getItemToolProperties(heldStack.item);

    if (!attackHeld || !hit.hit)
    {
        reset();
        return;
    }

    const mc::content::BlockState state = world.getBlockState(
        hit.blockPosition.x,
        hit.blockPosition.y,
        hit.blockPosition.z
    );
    const BlockType block = state.block();
    const std::uint8_t metadata = state.properties();

    if (!getBlockProperties(block).breakable)
    {
        reset();
        return;
    }

    if (!hasTarget_ || hit.blockPosition != blockPosition_)
    {
        blockPosition_ = hit.blockPosition;
        hasTarget_ = true;
        progress_ = 0.0f;
        tickAccumulator_ = 0.0;
        hitWaitTicks_ = 0;
    }

    // Clamp a single frame to avoid a pause or debugger stop causing many
    // seconds of block damage to be processed instantly.
    tickAccumulator_ +=
        static_cast<double>(std::clamp(deltaTime, 0.0f, 0.1f));

    int processedTicks = 0;
    while (tickAccumulator_ >= tickSeconds &&
           processedTicks < maximumCatchUpTicks)
    {
        tickAccumulator_ -= tickSeconds;
        ++processedTicks;

        if (hitWaitTicks_ > 0)
        {
            --hitWaitTicks_;
            continue;
        }

        progress_ += getBlockStrengthPerTick(
            block,
            tool,
            playerOnGround,
            playerUnderwater,
            hasteAmplifier,
            fatigueAmplifier,
            aquaAffinity
        );

        if (progress_ >= 1.0f)
        {
            const std::vector<ItemStack> containerContents =
                world.copyBlockEntityContents(
                    blockPosition_.x,
                    blockPosition_.y,
                    blockPosition_.z
                );

            if (world.setBlock(
                    blockPosition_.x,
                    blockPosition_.y,
                    blockPosition_.z,
                    BlockType::Air))
            {
                itemEntities.spawnBlockDrops(
                    block,
                    metadata,
                    tool,
                    blockPosition_
                );
                if (!containerContents.empty())
                {
                    itemEntities.spawnContainerDrops(
                        containerContents,
                        blockPosition_
                    );
                }

                // ItemTool.onBlockDestroyed consumes one use for every
                // successfully destroyed block, including soft blocks.
                if (isToolItem(heldStack.item))
                    heldStack.damageItem(1);
            }

            progress_ = 0.0f;
            tickAccumulator_ = 0.0;
            hitWaitTicks_ = 5;
            hasTarget_ = false;
            return;
        }
    }
}

void BlockBreakingController::reset() noexcept
{
    hasTarget_ = false;
    progress_ = 0.0f;
    tickAccumulator_ = 0.0;
    hitWaitTicks_ = 0;
}

bool BlockBreakingController::isBreaking() const noexcept
{
    return hasTarget_ && progress_ > 0.0f;
}

float BlockBreakingController::getProgress() const noexcept
{
    return progress_;
}

int BlockBreakingController::getDestroyStage() const noexcept
{
    if (!isBreaking())
        return -1;

    return std::clamp(
        static_cast<int>(progress_ * 10.0f),
        0,
        9
    );
}

const glm::ivec3& BlockBreakingController::getBlockPosition() const noexcept
{
    return blockPosition_;
}
