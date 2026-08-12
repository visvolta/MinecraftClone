#pragma once

#include "entity/Entity.h"

namespace mc::entity
{
class XpOrbEntity : public Entity
{
public:
    XpOrbEntity(World& world, double x, double y, double z, int xpValue);
    void onUpdate() override;
    void onCollideWithPlayer(PlayerEntity& player) override;
    [[nodiscard]] EntityKind entityKind() const noexcept override
    {
        return EntityKind::XpOrb;
    }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] int getXpValue() const noexcept { return xpValue_; }

private:
    int xpValue_ = 0;
    int delayBeforeCanPickup_ = 10;
};
}
