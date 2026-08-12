#pragma once

#include "Item.h"
#include "entity/Entity.h"

namespace mc::entity
{
class ItemEntityEntity : public Entity
{
public:
    ItemEntityEntity(
        World& world,
        double x,
        double y,
        double z,
        ItemStack stack
    );
    void onUpdate() override;
    void onCollideWithPlayer(PlayerEntity& player) override;
    [[nodiscard]] EntityKind entityKind() const noexcept override
    {
        return EntityKind::Item;
    }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] const ItemStack& getStack() const noexcept { return stack_; }
    [[nodiscard]] ItemStack& getStack() noexcept { return stack_; }
    void setPickupDelay(int ticks) noexcept { pickupDelay_ = ticks; }
    [[nodiscard]] int getAge() const noexcept { return ticksExisted_; }
    [[nodiscard]] float getHoverStart() const noexcept { return hoverStart_; }

private:
    ItemStack stack_{};
    int pickupDelay_ = 10;
    float hoverStart_ = 0.0f;
};
}
