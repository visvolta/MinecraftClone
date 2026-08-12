#pragma once

#include "entity/Mob.h"

namespace mc::entity
{
class SlimeEntity : public Mob
{
public:
    explicit SlimeEntity(World& world);
    void onUpdate() override;
    void setSlimeSize(int size, bool resetHealth);
    [[nodiscard]] int getSlimeSize() const noexcept { return size_; }
    [[nodiscard]] bool getCanSpawnHere() override;
    [[nodiscard]] EnumCreatureType getCreatureType() const override
    {
        return EnumCreatureType::Monster;
    }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    void onCollideWithPlayer(PlayerEntity& player) override;
    void onInitialSpawn() override;
    [[nodiscard]] float getEyeHeight() const override;

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
    void dealDamage(LivingEntity& entity);
    [[nodiscard]] int getAttackStrength() const noexcept { return size_; }

    int size_ = 1;
    float squishAmount_ = 0.0f;
    float squishFactor_ = 0.0f;
    float prevSquishFactor_ = 0.0f;
    bool wasOnGround_ = false;
};
}
