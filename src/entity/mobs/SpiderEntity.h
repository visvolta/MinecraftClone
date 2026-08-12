#pragma once

#include "entity/MonsterEntity.h"

namespace mc::entity
{
class SpiderEntity : public MonsterEntity
{
public:
    explicit SpiderEntity(World& world);
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] core::ResourceLocation getOverlayTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 0.65f; }
    [[nodiscard]] bool isOnLadder() const;

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
};

class CaveSpiderEntity : public SpiderEntity
{
public:
    explicit CaveSpiderEntity(World& world);
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getRenderScale() const override { return 0.7f; }
    [[nodiscard]] float getEyeHeight() const override { return 0.45f; }
    bool attackEntityAsMob(Entity& target) override;

protected:
    void applyEntityAttributes() override;
};
}
