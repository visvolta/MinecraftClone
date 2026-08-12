#pragma once

#include "entity/MonsterEntity.h"

namespace mc::entity
{
class ZombieEntity : public MonsterEntity
{
public:
    explicit ZombieEntity(World& world);

    void onLivingUpdate() override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override;
    [[nodiscard]] int getExperiencePoints(PlayerEntity* player) const override;
    [[nodiscard]] virtual bool shouldBurnInDay() const { return true; }
    void setChild(bool child);
    [[nodiscard]] bool isChild() const { return child_; }
    void onInitialSpawn() override;

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
    virtual void applyEntityAI();

    bool child_ = false;
};

class HuskEntity : public ZombieEntity
{
public:
    explicit HuskEntity(World& world);
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] bool shouldBurnInDay() const override { return false; }
    [[nodiscard]] float getRenderScale() const override { return 1.0625f; }
};

class ZombieVillagerEntity : public ZombieEntity
{
public:
    explicit ZombieVillagerEntity(World& world);
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
};
}
