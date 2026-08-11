#pragma once

#include "Item.h"
#include "core/ResourceLocation.h"
#include "entity/EntityUuid.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

class JavaRandom;
class Player;
class World;

namespace mc::gameplay { struct MobDefinition; }

namespace mc::entity
{
namespace ai
{
class MobAiController;
class VanillaGoal;
}

struct MobAnimationState
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

struct MobPersistentState
{
    core::ResourceLocation type{"minecraft:pig"};
    EntityUuid uuid{};
    EntityUuid ownerUuid{};
    EntityUuid loveCauseUuid{};
    EntityUuid leashHolderUuid{};
    glm::vec3 position{};
    glm::vec3 velocity{};
    float yaw = 0.0f;
    float health = 1.0f;
    int ticksExisted = 0;
    int growingAge = 0;
    int forcedAge = 0;
    int inLove = 0;
    int variant = 0;
    int temper = 0;
    bool tamed = false;
    bool sitting = false;
    bool sheared = false;
    bool saddled = false;
    bool leashed = false;
    ItemStack armor{};
};

struct MobBirthRequest
{
    MobPersistentState child;
};

class MobEntity;
class MobEntityManager;

struct MobTickContext
{
    World& world;
    Player& player;
    std::span<MobEntity> entities;
    std::vector<MobBirthRequest>& births;
    JavaRandom& random;
    ItemType playerMainHand = ItemType::Empty;
    bool daytime = true;
};

class MobEntity
{
public:
    MobEntity(
        core::ResourceLocation type,
        const gameplay::MobDefinition& definition,
        glm::vec3 position,
        float yaw
    );
    ~MobEntity();

    MobEntity(MobEntity&&) noexcept;
    MobEntity& operator=(MobEntity&&) noexcept;
    MobEntity(const MobEntity&) = delete;
    MobEntity& operator=(const MobEntity&) = delete;

    void tick(MobTickContext& context);
    void damage(float amount, bool causedByPlayer = false) noexcept;
    [[nodiscard]] bool interact(
        Player& player,
        ItemStack& heldStack,
        JavaRandom& random
    );

    [[nodiscard]] const core::ResourceLocation& type() const noexcept;
    [[nodiscard]] const core::ResourceLocation& texture() const noexcept;
    [[nodiscard]] const core::ResourceLocation& overlayTexture() const noexcept;
    [[nodiscard]] const glm::vec3& overlayColour() const noexcept;
    [[nodiscard]] const gameplay::MobDefinition& definition() const noexcept;
    [[nodiscard]] const glm::vec3& position() const noexcept;
    [[nodiscard]] glm::vec3 interpolatedPosition(float partialTick) const noexcept;
    [[nodiscard]] float yaw() const noexcept;
    [[nodiscard]] float interpolatedYaw(float partialTick) const noexcept;
    [[nodiscard]] float health() const noexcept;
    [[nodiscard]] int age() const noexcept;
    [[nodiscard]] bool dead() const noexcept;
    [[nodiscard]] bool killedByPlayer() const noexcept;
    [[nodiscard]] int deathTicks() const noexcept;
    [[nodiscard]] bool isChild() const noexcept;
    [[nodiscard]] bool isInLove() const noexcept;
    [[nodiscard]] bool isTamed() const noexcept;
    [[nodiscard]] bool isSitting() const noexcept;
    [[nodiscard]] bool isBegging() const noexcept;
    [[nodiscard]] bool isSheared() const noexcept;
    [[nodiscard]] const EntityUuid& uuid() const noexcept;
    [[nodiscard]] const EntityUuid& ownerUuid() const noexcept;
    [[nodiscard]] float renderScale() const noexcept;
    [[nodiscard]] float collisionWidth() const noexcept;
    [[nodiscard]] float collisionHeight() const noexcept;
    [[nodiscard]] float eyeHeight() const noexcept;
    [[nodiscard]] MobAnimationState animationState(
        float partialTick
    ) const noexcept;

    [[nodiscard]] MobPersistentState persistentState() const;
    void restorePersistentState(const MobPersistentState& state);

private:
    friend class ai::MobAiController;
    friend class ai::VanillaGoal;
    friend class MobEntityManager;

    core::ResourceLocation type_;
    EntityUuid uuid_ = EntityUuid::random();
    EntityUuid ownerUuid_{};
    EntityUuid loveCauseUuid_{};
    EntityUuid leashHolderUuid_{};
    EntityUuid riderUuid_{};
    EntityUuid caravanHeadUuid_{};
    EntityUuid caravanTailUuid_{};
    core::ResourceLocation texture_{"minecraft:entity/zombie/zombie"};
    core::ResourceLocation overlayTexture_{"minecraft:entity/empty"};
    glm::vec3 overlayColour_{1.0f};
    const gameplay::MobDefinition* definition_ = nullptr;
    std::unique_ptr<ai::MobAiController> ai_;
    glm::vec3 position_{};
    glm::vec3 previousPosition_{};
    glm::vec3 velocity_{};
    float yaw_ = 0.0f;
    float previousYaw_ = 0.0f;
    float headYaw_ = 0.0f;
    float headPitch_ = 0.0f;
    float limbSwing_ = 0.0f;
    float previousLimbSwing_ = 0.0f;
    float limbSwingAmount_ = 0.0f;
    float previousLimbSwingAmount_ = 0.0f;
    float health_ = 20.0f;
    float movementSpeedMultiplier_ = 0.0f;
    int age_ = 0;
    int attackCooldown_ = 0;
    int fireTicks_ = 0;
    int hurtTicks_ = 0;
    int panicTicks_ = 0;
    int revengeTicks_ = 0;
    int fuseTicks_ = 0;
    int deathTicks_ = 0;
    int eatTicks_ = 0;
    int growingAge_ = 0;
    int forcedAge_ = 0;
    int forcedAgeTimer_ = 0;
    int inLove_ = 0;
    int variant_ = 0;
    int temper_ = 0;
    bool onGround_ = false;
    bool inWater_ = false;
    bool inLava_ = false;
    bool aggressive_ = false;
    bool killedByPlayer_ = false;
    bool jumpRequested_ = false;
    bool tamed_ = false;
    bool sitting_ = false;
    bool begging_ = false;
    bool sheared_ = false;
    bool saddled_ = false;
    bool leashed_ = false;
    bool beingRidden_ = false;
    ItemStack armor_{};

    [[nodiscard]] bool collides(
        const World& world,
        const glm::vec3& position
    ) const;
    void moveAxis(const World& world, float amount, int axis);
    void updateAgeableState();
    void updateEnvironment(const World& world);
    void applyMovement(const World& world);
    void updateAnimation() noexcept;
    [[nodiscard]] bool isBreedingItem(ItemType item) const noexcept;
    [[nodiscard]] bool isTamingItem(ItemType item) const noexcept;
    [[nodiscard]] bool canMateWith(const MobEntity& other) const noexcept;
    static void consumeOne(ItemStack& stack) noexcept;
    void updateVariantPresentation();
    void rebindRuntime() noexcept;
};
}
