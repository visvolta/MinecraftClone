#include "ItemEntity.h"

#include "World.h"

#include <algorithm>
#include <cmath>

ItemEntity::ItemEntity(
    const glm::vec3& position,
    ItemStack stack,
    const glm::vec3& velocity,
    float hoverStart,
    int pickupDelay)
    : position_(position),
      previousPosition_(position),
      velocity_(velocity),
      stack_(stack),
      pickupDelay_(std::max(0, pickupDelay)),
      hoverStart_(hoverStart)
{
}

void ItemEntity::tick(const World& world)
{
    if (dead_)
        return;

    if (pickupDelay_ > 0)
        --pickupDelay_;

    previousPosition_ = position_;
    velocity_.y -= 0.04f;
    onGround_ = false;

    moveAxis(world, velocity_.x, 0);
    moveAxis(world, velocity_.y, 1);
    moveAxis(world, velocity_.z, 2);

    const float horizontalDrag = onGround_ ? 0.58800006f : 0.98f;
    velocity_.x *= horizontalDrag;
    velocity_.y *= 0.98f;
    velocity_.z *= horizontalDrag;

    if (onGround_)
        velocity_.y *= -0.5f;

    ++age_;
    if (age_ >= 6000)
        dead_ = true;
}

bool ItemEntity::isDead() const noexcept
{
    return dead_ || stack_.empty();
}

void ItemEntity::kill() noexcept
{
    dead_ = true;
}

bool ItemEntity::canBePickedUp() const noexcept
{
    return pickupDelay_ == 0;
}

bool ItemEntity::isNear(const glm::vec3& point) const noexcept
{
    const glm::vec3 delta = position_ - point;
    return glm::dot(delta, delta) <= 1.5f * 1.5f;
}

const glm::vec3& ItemEntity::getPosition() const noexcept
{
    return position_;
}

const glm::vec3& ItemEntity::getPreviousPosition() const noexcept
{
    return previousPosition_;
}

glm::vec3 ItemEntity::getInterpolatedPosition(float partialTick) const
{
    return previousPosition_ +
           (position_ - previousPosition_) *
           std::clamp(partialTick, 0.0f, 1.0f);
}

const ItemStack& ItemEntity::getStack() const noexcept
{
    return stack_;
}

ItemStack& ItemEntity::getStack() noexcept
{
    return stack_;
}

int ItemEntity::getAge() const noexcept
{
    return age_;
}

float ItemEntity::getHoverStart() const noexcept
{
    return hoverStart_;
}

bool ItemEntity::collides(
    const World& world,
    const glm::vec3& position) const
{
    const int minX =
        static_cast<int>(std::floor(position.x - HALF_SIZE));
    const int maxX =
        static_cast<int>(std::floor(position.x + HALF_SIZE));
    const int minY =
        static_cast<int>(std::floor(position.y - HALF_SIZE));
    const int maxY =
        static_cast<int>(std::floor(position.y + HALF_SIZE));
    const int minZ =
        static_cast<int>(std::floor(position.z - HALF_SIZE));
    const int maxZ =
        static_cast<int>(std::floor(position.z + HALF_SIZE));

    for (int z = minZ; z <= maxZ; ++z)
    {
        for (int y = minY; y <= maxY; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                if (world.isSolidBlock(x, y, z))
                    return true;
            }
        }
    }

    return false;
}

void ItemEntity::moveAxis(
    const World& world,
    float amount,
    int axis)
{
    if (amount == 0.0f)
        return;

    glm::vec3 candidate = position_;
    candidate[axis] += amount;

    if (!collides(world, candidate))
    {
        position_ = candidate;
        return;
    }

    if (axis == 1 && amount < 0.0f)
        onGround_ = true;

    velocity_[axis] = 0.0f;
}
const mc::core::ResourceLocation& ItemEntity::typeId() noexcept
{
    static const mc::core::ResourceLocation id("minecraft:item");
    return id;
}
