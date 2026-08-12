#pragma once

#include "entity/MonsterEntity.h"

namespace mc::entity
{
class WitchEntity : public MonsterEntity
{
public:
    explicit WitchEntity(World& world);
    bool attackEntityAsMob(Entity& target) override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 1.62f; }

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
};
}
