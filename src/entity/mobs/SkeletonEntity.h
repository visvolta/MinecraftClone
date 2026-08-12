#pragma once

#include "entity/MonsterEntity.h"

namespace mc::entity
{
class AbstractSkeletonEntity : public MonsterEntity
{
public:
    explicit AbstractSkeletonEntity(World& world);
    void onLivingUpdate() override;
    bool attackEntityAsMob(Entity& target) override;
    [[nodiscard]] float getEyeHeight() const override { return 1.74f; }
    [[nodiscard]] virtual bool shouldBurnInDay() const { return true; }

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
};

class SkeletonEntity : public AbstractSkeletonEntity
{
public:
    explicit SkeletonEntity(World& world);
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
};

class StrayEntity : public AbstractSkeletonEntity
{
public:
    explicit StrayEntity(World& world);
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] core::ResourceLocation getOverlayTexture() const override;
};
}
