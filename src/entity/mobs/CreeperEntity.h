#pragma once

#include "entity/MonsterEntity.h"

namespace mc::entity
{
class CreeperEntity : public MonsterEntity
{
public:
    explicit CreeperEntity(World& world);

    void onUpdate() override;
    void fall(float distance, float damageMultiplier) override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 1.7f * 0.85f; }
    [[nodiscard]] float getAttackProgress() const override;
    [[nodiscard]] int getMaxFallHeight() const;
    void setCreeperState(int state) noexcept { creeperState_ = state; }
    [[nodiscard]] int getCreeperState() const noexcept { return creeperState_; }
    void explode();

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;

private:
    int lastActiveTime_ = 0;
    int timeSinceIgnited_ = 0;
    int fuseTime_ = 30;
    int explosionRadius_ = 3;
    int creeperState_ = -1;
};
}
