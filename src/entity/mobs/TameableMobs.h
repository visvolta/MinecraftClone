#pragma once

#include "entity/TameableEntity.h"
#include "entity/AnimalEntity.h"

namespace mc::entity
{
class WolfEntity : public TameableEntity
{
public:
    explicit WolfEntity(World& world);
    void setTamed(bool tamed) override;
    void setAttackTarget(LivingEntity* target);
    bool attackEntityAsMob(Entity& target) override;
    bool processInteract(PlayerEntity& player, ItemStack& stack);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] bool isBreedingItem(ItemType item) const override;
    [[nodiscard]] bool isBegging() const override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 0.68f; }

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
    bool begging_ = false;
};

class OcelotEntity : public TameableEntity
{
public:
    explicit OcelotEntity(World& world);
    bool processInteract(PlayerEntity& player, ItemStack& stack);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] bool isBreedingItem(ItemType item) const override;
    [[nodiscard]] bool getCanSpawnHere() override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 0.595f; }
    void onInitialSpawn() override;

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
    int variant_ = 0;
};

class ParrotEntity : public TameableEntity
{
public:
    explicit ParrotEntity(World& world);
    bool processInteract(PlayerEntity& player, ItemStack& stack);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] bool isBreedingItem(ItemType) const override { return false; }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 0.756f; }
    void onInitialSpawn() override;

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
    int variant_ = 0;
};

class AbstractHorseEntity : public AnimalEntity
{
public:
    explicit AbstractHorseEntity(World& world);
    bool processInteract(PlayerEntity& player, ItemStack& stack);
    void onLivingUpdate() override;
    [[nodiscard]] bool isBreedingItem(ItemType item) const override;
    [[nodiscard]] bool canDespawn() const override { return !tamed_; }
    void setTamed(bool tamed) noexcept { tamed_ = tamed; }
    [[nodiscard]] bool isTamed() const noexcept { return tamed_; }
    void setHorseSaddled(bool saddled) noexcept { saddled_ = saddled; }
    [[nodiscard]] bool isHorseSaddled() const noexcept { return saddled_; }
    [[nodiscard]] int getTemper() const noexcept { return temper_; }
    [[nodiscard]] virtual int getMaxTemper() const noexcept { return 100; }
    [[nodiscard]] float getEyeHeight() const override { return 1.4f; }
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override
    {
        return gameplay::MobModelKind::Horse;
    }

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
    bool tamed_ = false;
    bool saddled_ = false;
    int temper_ = 0;
};

class HorseEntity : public AbstractHorseEntity
{
public:
    explicit HorseEntity(World& world);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    void onInitialSpawn() override;

protected:
    void applyEntityAttributes() override;
    int variant_ = 0;
};

class DonkeyEntity : public AbstractHorseEntity
{
public:
    explicit DonkeyEntity(World& world);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
};

class MuleEntity : public AbstractHorseEntity
{
public:
    explicit MuleEntity(World& world);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
};

class SkeletonHorseEntity : public AbstractHorseEntity
{
public:
    explicit SkeletonHorseEntity(World& world);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] bool isBreedingItem(ItemType) const override { return false; }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
};

class ZombieHorseEntity : public AbstractHorseEntity
{
public:
    explicit ZombieHorseEntity(World& world);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] bool isBreedingItem(ItemType) const override { return false; }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
};

class LlamaEntity : public AbstractHorseEntity
{
public:
    explicit LlamaEntity(World& world);
    bool attackEntityAsMob(Entity& target) override;
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] int getMaxTemper() const noexcept override { return 30; }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    void onInitialSpawn() override;

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
    int variant_ = 0;
};
}
