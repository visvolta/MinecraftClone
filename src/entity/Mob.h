#pragma once

#include "entity/LivingEntity.h"
#include "entity/Math.h"
#include "entity/EnumCreatureType.h"
#include "entity/ai/Goal.h"
#include "entity/helpers/BodyHelper.h"
#include "entity/helpers/JumpHelper.h"
#include "entity/helpers/LookHelper.h"
#include "entity/helpers/MoveHelper.h"
#include "entity/helpers/Senses.h"
#include "entity/navigation/PathNavigation.h"
#include "gameplay/GameplayRegistries.h"

#include <memory>

namespace mc::entity
{
class Mob : public LivingEntity, public ai::GoalContext
{
public:
    explicit Mob(World& world);

    void onUpdate() override;
    void updateEntityActionState() override;
    [[nodiscard]] bool canBePushed() const override { return isAlive(); }

    LookHelper& getLookHelper() noexcept { return lookHelper_; }
    MoveHelper& getMoveHelper() noexcept { return moveHelper_; }
    JumpHelper& getJumpHelper() noexcept { return jumpHelper_; }
    EntitySenses& getEntitySenses() noexcept { return senses_; }
    navigation::PathNavigation& getNavigator() noexcept { return navigator_; }
    const navigation::PathNavigation& getNavigator() const noexcept
    {
        return navigator_;
    }
    ai::GoalSelector& goalSelector() noexcept { return tasks_; }
    ai::GoalSelector& targetSelector() noexcept { return targetTasks_; }

    [[nodiscard]] Mob* mob() override { return this; }

    void setAttackTarget(LivingEntity* target);
    [[nodiscard]] LivingEntity* getAttackTarget() const noexcept
    {
        return attackTarget_;
    }
    void setAIMoveSpeed(float speed) noexcept override;
    void clearDeadEntityReferences(const Entity* removed) override;

    virtual bool attackEntityAsMob(Entity& target);
    virtual void eatGrassBonus() {}
    [[nodiscard]] virtual bool getCanSpawnHere();
    [[nodiscard]] virtual bool isNotColliding() const;
    virtual void onInitialSpawn();
    [[nodiscard]] virtual bool canDespawn() const { return true; }
    [[nodiscard]] virtual int getMaxSpawnedInChunk() const { return 4; }
    [[nodiscard]] virtual EnumCreatureType getCreatureType() const
    {
        return EnumCreatureType::Creature;
    }
    [[nodiscard]] virtual gameplay::MobModelKind getModelKind() const
    {
        return gameplay::MobModelKind::Biped;
    }
    [[nodiscard]] virtual core::ResourceLocation getTexture() const;
    [[nodiscard]] virtual core::ResourceLocation getOverlayTexture() const;
    [[nodiscard]] virtual glm::vec3 getOverlayColour() const { return {1, 1, 1}; }
    [[nodiscard]] virtual float getRenderScale() const { return 1.0f; }
    [[nodiscard]] virtual bool isSheared() const { return false; }
    [[nodiscard]] virtual bool isSitting() const { return false; }
    [[nodiscard]] virtual bool isBegging() const { return false; }
    [[nodiscard]] virtual bool isAggressive() const
    {
        return attackTarget_ != nullptr;
    }
    [[nodiscard]] virtual float getAttackProgress() const { return 0.0f; }
    [[nodiscard]] bool getLeashed() const noexcept { return leashed_; }
    void setLeashed(bool value, Entity* holder);
    [[nodiscard]] Entity* getLeashHolder() const noexcept { return leashHolder_; }
    void setPersistenceRequired(bool value) noexcept { persistenceRequired_ = value; }
    [[nodiscard]] bool isPersistenceRequired() const noexcept
    {
        return persistenceRequired_;
    }
    [[nodiscard]] float getPathPriority(navigation::PathNodeType type) const;
    void setPathPriority(navigation::PathNodeType type, float value);
    [[nodiscard]] virtual float getBlockPathWeight(int x, int y, int z) const;
    [[nodiscard]] float getBrightness() const override;

    void enablePersistence() noexcept { persistenceRequired_ = true; }
    void faceEntity(Entity& entity, float maxYaw, float maxPitch);

    [[nodiscard]] float interpolatedYaw(float partialTick) const;
    struct PoseState
    {
        float age = 0.0f;
        float limbSwing = 0.0f;
        float limbSwingAmount = 0.0f;
        float headYaw = 0.0f;
        float headPitch = 0.0f;
        float attackProgress = 0.0f;
        float jumpProgress = 0.0f;
        float hurtProgress = 0.0f;
        float deathProgress = 0.0f;
        bool onGround = true;
        bool inWater = false;
        bool aggressive = false;
        bool child = false;
        bool sitting = false;
        bool begging = false;
    };
    [[nodiscard]] PoseState poseState(float partialTick) const;

protected:
    virtual void initEntityAI() {}
    virtual void applyEntityAttributes();
    void updateRenderYawOffset() override;
    void despawnEntity();
    void updateLeashedState();
    [[nodiscard]] virtual navigation::NavigationSettings createNavigationSettings() const;

    ai::GoalSelector tasks_;
    ai::GoalSelector targetTasks_;
    LookHelper lookHelper_;
    MoveHelper moveHelper_;
    JumpHelper jumpHelper_;
    BodyHelper bodyHelper_;
    navigation::PathNavigation navigator_;
    EntitySenses senses_;
    LivingEntity* attackTarget_ = nullptr;
    bool persistenceRequired_ = false;
    bool leashed_ = false;
    Entity* leashHolder_ = nullptr;
    float pathPriorities_[static_cast<int>(navigation::PathNodeType::Count)]{};
};
}
