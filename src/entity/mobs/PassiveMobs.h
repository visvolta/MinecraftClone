#pragma once

#include "entity/AnimalEntity.h"
#include "entity/TameableEntity.h"
#include "entity/Mob.h"

namespace mc::entity
{
class CowEntity : public AnimalEntity
{
public:
    explicit CowEntity(World& world);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 1.3f; }

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
};

class MooshroomEntity : public CowEntity
{
public:
    explicit MooshroomEntity(World& world);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] bool getCanSpawnHere() override;
};

class PigEntity : public AnimalEntity
{
public:
    explicit PigEntity(World& world);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] bool isBreedingItem(ItemType item) const override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 0.765f; }

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
};

class ChickenEntity : public AnimalEntity
{
public:
    explicit ChickenEntity(World& world);
    void onLivingUpdate() override;
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] bool isBreedingItem(ItemType item) const override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 0.644f; }

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
    int timeUntilNextEgg_ = 0;
};

class RabbitEntity : public AnimalEntity
{
public:
    explicit RabbitEntity(World& world);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] bool isBreedingItem(ItemType item) const override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 0.425f; }
    void onInitialSpawn() override;

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
    int variant_ = 0;
};

class SheepEntity : public AnimalEntity
{
public:
    explicit SheepEntity(World& world);
    void eatGrassBonus() override;
    bool processInteract(PlayerEntity& player, ItemStack& stack);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] bool isSheared() const override { return sheared_; }
    [[nodiscard]] int getFleeceColor() const noexcept { return fleeceColor_; }
    void setFleeceColor(int color) noexcept { fleeceColor_ = color; }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] core::ResourceLocation getOverlayTexture() const override;
    [[nodiscard]] glm::vec3 getOverlayColour() const override;
    [[nodiscard]] float getEyeHeight() const override { return 1.235f; }
    void onInitialSpawn() override;
    static int getRandomSheepColor(JavaRandom& random);

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
    bool sheared_ = false;
    int fleeceColor_ = 0;
};

class BatEntity : public Mob
{
public:
    explicit BatEntity(World& world);
    void onUpdate() override;
    [[nodiscard]] bool getCanSpawnHere() override;
    [[nodiscard]] EnumCreatureType getCreatureType() const override
    {
        return EnumCreatureType::Ambient;
    }
    [[nodiscard]] bool canDespawn() const override { return true; }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 0.45f; }

protected:
    void applyEntityAttributes() override;
    bool hanging_ = false;
};

class SquidEntity : public Mob
{
public:
    explicit SquidEntity(World& world);
    void onLivingUpdate() override;
    [[nodiscard]] bool getCanSpawnHere() override;
    [[nodiscard]] EnumCreatureType getCreatureType() const override
    {
        return EnumCreatureType::WaterCreature;
    }
    [[nodiscard]] bool canBreatheUnderwater() const { return true; }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 0.4f; }

protected:
    void applyEntityAttributes() override;
};

class IronGolemEntity : public Creature
{
public:
    explicit IronGolemEntity(World& world);
    bool attackEntityAsMob(Entity& target) override;
    [[nodiscard]] bool canDespawn() const override { return false; }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 2.295f; }

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
};

class SnowGolemEntity : public Creature
{
public:
    explicit SnowGolemEntity(World& world);
    void onLivingUpdate() override;
    bool attackEntityAsMob(Entity& target) override;
    [[nodiscard]] bool canDespawn() const override { return false; }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 1.7f; }

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
};

class PolarBearEntity : public AnimalEntity
{
public:
    explicit PolarBearEntity(World& world);
    [[nodiscard]] std::unique_ptr<AgeableEntity> createChild(AnimalEntity&) override;
    [[nodiscard]] bool isBreedingItem(ItemType) const override { return false; }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 1.19f; }

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
};
}
