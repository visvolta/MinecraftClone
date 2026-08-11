#include "entity/MobEntity.h"

#include "BlockProperties.h"
#include "BlockShape.h"
#include "Player.h"
#include "World.h"
#include "entity/ai/MobAiController.h"
#include "gameplay/GameplayRegistries.h"
#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numbers>
#include <utility>

namespace mc::entity
{
namespace
{
bool intersects(
    const glm::vec3& entityMinimum,
    const glm::vec3& entityMaximum,
    const BlockBox& block,
    const glm::ivec3& blockPosition)
{
    const glm::vec3 minimum = glm::vec3(blockPosition) + block.minimum;
    const glm::vec3 maximum = glm::vec3(blockPosition) + block.maximum;
    return entityMaximum.x > minimum.x && entityMinimum.x < maximum.x &&
           entityMaximum.y > minimum.y && entityMinimum.y < maximum.y &&
           entityMaximum.z > minimum.z && entityMinimum.z < maximum.z;
}

bool isLiquid(BlockType block) noexcept
{
    return block == BlockType::Water || block == BlockType::Lava;
}
}

MobEntity::MobEntity(
    core::ResourceLocation type,
    const gameplay::MobDefinition& definition,
    glm::vec3 position,
    float yaw)
    : type_(std::move(type)),
      definition_(&definition),
      position_(position),
      previousPosition_(position),
      yaw_(yaw),
      previousYaw_(yaw),
      health_(definition.maximumHealth)
{
    std::size_t variantSeed = std::hash<std::string>{}(type_.toString());
    variantSeed ^= static_cast<std::size_t>(
        std::floor(position.x) * 73428767.0f +
        std::floor(position.z) * 912931.0f
    );
    variant_ = static_cast<int>(variantSeed & 0x7fffffffU);
    updateVariantPresentation();
    ai_ = std::make_unique<ai::MobAiController>(*this);
}

MobEntity::~MobEntity() = default;
MobEntity::MobEntity(MobEntity&&) noexcept = default;
MobEntity& MobEntity::operator=(MobEntity&&) noexcept = default;

void MobEntity::tick(MobTickContext& context)
{
    previousPosition_ = position_;
    previousYaw_ = yaw_;
    previousLimbSwing_ = limbSwing_;
    previousLimbSwingAmount_ = limbSwingAmount_;

    if (dead())
    {
        if (beingRidden_ && riderUuid_ == context.player.uuid())
        {
            beingRidden_ = false;
            riderUuid_ = {};
            context.player.stopRiding();
        }
        ++deathTicks_;
        return;
    }

    ++age_;
    if (attackCooldown_ > 0) --attackCooldown_;
    if (hurtTicks_ > 0) --hurtTicks_;
    if (panicTicks_ > 0) --panicTicks_;
    if (revengeTicks_ > 0) --revengeTicks_;
    if (inLove_ > 0) --inLove_;
    updateAgeableState();
    updateEnvironment(context.world);

    if (leashed_)
    {
        if (leashHolderUuid_ != context.player.uuid() ||
            !context.player.isAlive())
        {
            leashed_ = false;
            leashHolderUuid_ = {};
        }
        else
        {
            const glm::vec3 delta = context.player.getPosition() - position_;
            const float distance = std::sqrt(glm::dot(delta, delta));
            if (distance > 10.0f)
            {
                leashed_ = false;
                leashHolderUuid_ = {};
            }
            else if (distance > 6.0f && distance > 0.0001f)
            {
                const glm::vec3 direction = delta / distance;
                velocity_.x += direction.x * std::abs(direction.x) * 0.4f;
                velocity_.y += direction.y * std::abs(direction.y) * 0.4f;
                velocity_.z += direction.z * std::abs(direction.z) * 0.4f;
            }
            else if (distance > 2.0f && !sitting_)
            {
                static_cast<void>(ai_->navigateTo(
                    context.world,
                    position_ + delta / std::max(distance, 0.0001f) *
                        (distance - 2.0f),
                    1.0
                ));
            }
        }
    }

    if (beingRidden_ && riderUuid_ == context.player.uuid() &&
        context.player.isCrouching())
    {
        beingRidden_ = false;
        riderUuid_ = {};
        context.player.stopRiding();
    }

    jumpRequested_ = false;
    ai_->tick(context);
    applyMovement(context.world);
    if (beingRidden_ && riderUuid_ == context.player.uuid())
    {
        context.player.setRidingPosition(
            position_ + glm::vec3(
                0.0f, collisionHeight() * 0.75f - 0.35f, 0.0f
            )
        );
    }

    if (definition_->burnsInDaylight && context.daytime)
    {
        const glm::ivec3 eyes(glm::floor(
            position_ + glm::vec3(0.0f, eyeHeight(), 0.0f)
        ));
        if (context.world.getSkyLightLevel(eyes.x, eyes.y, eyes.z) == 15)
            fireTicks_ = std::max(fireTicks_, 160);
    }
    if (fireTicks_ > 0)
    {
        --fireTicks_;
        if (fireTicks_ % 20 == 0)
            damage(1.0f);
    }

    updateAnimation();
}

void MobEntity::damage(float amount, bool causedByPlayer) noexcept
{
    if (dead() || amount <= 0.0f)
        return;
    health_ = std::max(0.0f, health_ - amount);
    hurtTicks_ = 10;
    if (definition_ != nullptr && gameplay::hasAiGoal(
            definition_->aiGoals, gameplay::MobAiGoal::Panic))
        panicTicks_ = 100;
    if (causedByPlayer)
    {
        revengeTicks_ = 400;
        if (health_ <= 0.0f)
            killedByPlayer_ = true;
    }
}

bool MobEntity::interact(
    Player& player,
    ItemStack& heldStack,
    JavaRandom& random)
{
    const ItemType held = heldStack.item;

    if (held == ItemType::Lead && !leashed_ &&
        definition_->category != gameplay::MobCategory::Monster)
    {
        leashed_ = true;
        leashHolderUuid_ = player.uuid();
        consumeOne(heldStack);
        return true;
    }

    if (type_.path() == "sheep" && held == ItemType::Shears &&
        !sheared_ && !isChild())
    {
        sheared_ = true;
        heldStack.damageItem(1);
        return true;
    }

    if (definition_->tameableKind == gameplay::TameableKind::Wolf)
    {
        if (tamed_)
        {
            if (isBreedingItem(held) && health_ < definition_->maximumHealth)
            {
                health_ = std::min(
                    definition_->maximumHealth,
                    health_ + static_cast<float>(
                        std::max(1, getItemProperties(held).foodPoints)
                    )
                );
                consumeOne(heldStack);
                return true;
            }
            if (ownerUuid_ == player.uuid() && !isBreedingItem(held))
            {
                sitting_ = !sitting_;
                ai_->clearPath();
                return true;
            }
        }
        else if (held == ItemType::Bone)
        {
            consumeOne(heldStack);
            if (random.nextInt(3) == 0)
            {
                tamed_ = true;
                ownerUuid_ = player.uuid();
                sitting_ = true;
                health_ = definition_->maximumHealth;
                ai_->clearPath();
            }
            return true;
        }
    }

    if (definition_->tameableKind == gameplay::TameableKind::Ocelot &&
        tamed_ && ownerUuid_ == player.uuid() && !isBreedingItem(held))
    {
        sitting_ = !sitting_;
        ai_->clearPath();
        return true;
    }

    if (definition_->tameableKind == gameplay::TameableKind::Ocelot &&
        !tamed_ && held == ItemType::RawFish)
    {
        const glm::vec3 delta = player.getPosition() - position_;
        if (glm::dot(delta, delta) >= 9.0f)
            return false;
        consumeOne(heldStack);
        if (random.nextInt(3) == 0)
        {
            tamed_ = true;
            ownerUuid_ = player.uuid();
            variant_ = 1 + random.nextInt(3);
            sitting_ = true;
            updateVariantPresentation();
        }
        return true;
    }

    if (definition_->tameableKind == gameplay::TameableKind::Parrot)
    {
        if (!tamed_ && isTamingItem(held))
        {
            consumeOne(heldStack);
            if (random.nextInt(10) == 0)
            {
                tamed_ = true;
                ownerUuid_ = player.uuid();
            }
            return true;
        }
        if (held == ItemType::Cookie)
        {
            consumeOne(heldStack);
            damage(1000.0f, true);
            return true;
        }
        if (tamed_ && ownerUuid_ == player.uuid())
        {
            sitting_ = !sitting_;
            ai_->clearPath();
            return true;
        }
    }

    const gameplay::TameableKind tameableKind = definition_->tameableKind;
    const bool regularEquine =
        tameableKind == gameplay::TameableKind::Horse ||
        tameableKind == gameplay::TameableKind::Donkey ||
        tameableKind == gameplay::TameableKind::Mule;
    const bool llama = tameableKind == gameplay::TameableKind::Llama;
    const bool undeadEquine =
        tameableKind == gameplay::TameableKind::SkeletonHorse ||
        tameableKind == gameplay::TameableKind::ZombieHorse;
    if (regularEquine || llama || undeadEquine)
    {
        int growth = 0;
        int temper = 0;
        float heal = 0.0f;
        bool love = false;
        if (!undeadEquine)
        {
            switch (held)
            {
                case ItemType::WheatItem:
                    growth = llama ? 10 : 20;
                    temper = 3;
                    heal = 2.0f;
                    break;
                case ItemType::Sugar:
                    if (!llama)
                    {
                        growth = 30;
                        temper = 3;
                        heal = 1.0f;
                    }
                    break;
                case ItemType::Apple:
                    if (!llama)
                    {
                        growth = 60;
                        temper = 3;
                        heal = 3.0f;
                    }
                    break;
                case ItemType::GoldenCarrot:
                    if (!llama)
                    {
                        growth = 60;
                        temper = 5;
                        heal = 4.0f;
                        love = true;
                    }
                    break;
                case ItemType::GoldenApple:
                    if (!llama)
                    {
                        growth = 240;
                        temper = 10;
                        heal = 10.0f;
                        love = true;
                    }
                    break;
                default:
                    if (held == itemFromBlock(BlockType::HayBale))
                    {
                        growth = llama ? 90 : 180;
                        temper = llama ? 6 : 0;
                        heal = llama ? 10.0f : 20.0f;
                        love = llama;
                    }
                    break;
            }
        }
        bool changed = false;
        if (health_ < definition_->maximumHealth && heal > 0.0f)
        {
            health_ = std::min(definition_->maximumHealth, health_ + heal);
            changed = true;
        }
        if (isChild() && growth > 0)
        {
            growingAge_ = std::min(0, growingAge_ + growth * 20);
            changed = true;
        }
        if (love && tamed_ && growingAge_ == 0 && inLove_ <= 0)
        {
            inLove_ = 600;
            loveCauseUuid_ = player.uuid();
            changed = true;
        }
        if (temper > 0 && (changed || !tamed_) &&
            temper_ < definition_->maximumTemper)
        {
            temper_ = std::min(definition_->maximumTemper, temper_ + temper);
            changed = true;
        }
        if (changed)
        {
            consumeOne(heldStack);
            return true;
        }
        if (tamed_ && !isChild() && held == ItemType::Saddle && !saddled_)
        {
            saddled_ = true;
            consumeOne(heldStack);
            return true;
        }
        if (tamed_ && tameableKind == gameplay::TameableKind::Horse &&
            armor_.empty() &&
            (held == ItemType::IronHorseArmor ||
             held == ItemType::GoldenHorseArmor ||
             held == ItemType::DiamondHorseArmor))
        {
            armor_ = {held, 1};
            consumeOne(heldStack);
            return true;
        }
        if (heldStack.empty() && !isChild() &&
            (!undeadEquine || tamed_))
        {
            beingRidden_ = true;
            riderUuid_ = player.uuid();
            sitting_ = false;
            player.startRiding();
            return true;
        }
    }

    if (definition_->breedable && isBreedingItem(held))
    {
        if (isChild())
        {
            const int seconds = static_cast<int>(
                static_cast<float>(-growingAge_ / 20) * 0.1f
            );
            const int oldAge = growingAge_;
            growingAge_ = std::min(
                0, growingAge_ + std::max(1, seconds) * 20
            );
            forcedAge_ += growingAge_ - oldAge;
            if (forcedAgeTimer_ == 0)
                forcedAgeTimer_ = 40;
            if (growingAge_ == 0)
                growingAge_ = forcedAge_;
            consumeOne(heldStack);
            return true;
        }
        if (growingAge_ == 0 && inLove_ <= 0)
        {
            inLove_ = 600;
            loveCauseUuid_ = player.uuid();
            consumeOne(heldStack);
            return true;
        }
    }
    return false;
}

const core::ResourceLocation& MobEntity::type() const noexcept { return type_; }
const core::ResourceLocation& MobEntity::texture() const noexcept
{
    return texture_;
}
const core::ResourceLocation& MobEntity::overlayTexture() const noexcept
{
    return overlayTexture_;
}
const glm::vec3& MobEntity::overlayColour() const noexcept
{
    return overlayColour_;
}
const gameplay::MobDefinition& MobEntity::definition() const noexcept
{
    return *definition_;
}
const glm::vec3& MobEntity::position() const noexcept { return position_; }
glm::vec3 MobEntity::interpolatedPosition(float partialTick) const noexcept
{
    const float partial = std::clamp(partialTick, 0.0f, 1.0f);
    return previousPosition_ + (position_ - previousPosition_) * partial;
}
float MobEntity::yaw() const noexcept { return yaw_; }
float MobEntity::interpolatedYaw(float partialTick) const noexcept
{
    const float delta = std::remainder(
        yaw_ - previousYaw_, std::numbers::pi_v<float> * 2.0f
    );
    return previousYaw_ + delta * std::clamp(partialTick, 0.0f, 1.0f);
}
float MobEntity::health() const noexcept { return health_; }
int MobEntity::age() const noexcept { return age_; }
bool MobEntity::dead() const noexcept { return health_ <= 0.0f; }
bool MobEntity::killedByPlayer() const noexcept { return killedByPlayer_; }
int MobEntity::deathTicks() const noexcept { return deathTicks_; }
bool MobEntity::isChild() const noexcept { return growingAge_ < 0; }
bool MobEntity::isInLove() const noexcept
{
    return definition_->breedable && growingAge_ == 0 && inLove_ > 0;
}
bool MobEntity::isTamed() const noexcept { return tamed_; }
bool MobEntity::isSitting() const noexcept { return sitting_; }
bool MobEntity::isBegging() const noexcept { return begging_; }
bool MobEntity::isSheared() const noexcept { return sheared_; }
const EntityUuid& MobEntity::uuid() const noexcept { return uuid_; }
const EntityUuid& MobEntity::ownerUuid() const noexcept { return ownerUuid_; }
float MobEntity::renderScale() const noexcept
{
    return definition_->renderScale * (isChild() ? 0.5f : 1.0f);
}
float MobEntity::collisionWidth() const noexcept
{
    return definition_->width * (isChild() ? 0.5f : 1.0f);
}
float MobEntity::collisionHeight() const noexcept
{
    return definition_->height * (isChild() ? 0.5f : 1.0f);
}
float MobEntity::eyeHeight() const noexcept
{
    return definition_->eyeHeight * (isChild() ? 0.5f : 1.0f);
}

MobAnimationState MobEntity::animationState(float partialTick) const noexcept
{
    const float partial = std::clamp(partialTick, 0.0f, 1.0f);
    const float combatProgress = definition_->attackIntervalTicks > 0 &&
            attackCooldown_ > 0
        ? 1.0f - static_cast<float>(attackCooldown_) /
            static_cast<float>(definition_->attackIntervalTicks)
        : definition_->attackKind == gameplay::MobAttackKind::CreeperExplosion
            ? static_cast<float>(fuseTicks_) / 30.0f
            : 0.0f;
    const float attackProgress = eatTicks_ > 0
        ? 1.0f - std::abs(static_cast<float>(eatTicks_ - 20)) / 20.0f
        : combatProgress;
    return {
        static_cast<float>(age_) + partial,
        previousLimbSwing_ + (limbSwing_ - previousLimbSwing_) * partial,
        previousLimbSwingAmount_ +
            (limbSwingAmount_ - previousLimbSwingAmount_) * partial,
        headYaw_, headPitch_, std::clamp(attackProgress, 0.0f, 1.0f),
        onGround_ ? 0.0f : std::clamp(std::abs(velocity_.y) * 2.0f, 0.0f, 1.0f),
        static_cast<float>(hurtTicks_) / 10.0f,
        std::clamp((static_cast<float>(deathTicks_) + partial) / 20.0f,
                   0.0f, 1.0f),
        onGround_, inWater_, aggressive_
    };
}

MobPersistentState MobEntity::persistentState() const
{
    return {
        type_, uuid_, ownerUuid_, loveCauseUuid_, leashHolderUuid_,
        position_, velocity_, yaw_, health_, age_,
        growingAge_, forcedAge_, inLove_, variant_, temper_, tamed_, sitting_,
        sheared_, saddled_, leashed_, armor_
    };
}

void MobEntity::restorePersistentState(const MobPersistentState& state)
{
    uuid_ = state.uuid.empty() ? EntityUuid::random() : state.uuid;
    ownerUuid_ = state.ownerUuid;
    loveCauseUuid_ = state.loveCauseUuid;
    leashHolderUuid_ = state.leashHolderUuid;
    position_ = state.position;
    previousPosition_ = state.position;
    velocity_ = state.velocity;
    yaw_ = state.yaw;
    previousYaw_ = state.yaw;
    health_ = std::clamp(state.health, 0.0f, definition_->maximumHealth);
    age_ = std::max(0, state.ticksExisted);
    growingAge_ = state.growingAge;
    forcedAge_ = state.forcedAge;
    inLove_ = std::max(0, state.inLove);
    variant_ = state.variant;
    temper_ = std::clamp(state.temper, 0, definition_->maximumTemper);
    tamed_ = state.tamed;
    sitting_ = state.sitting;
    sheared_ = state.sheared;
    saddled_ = state.saddled;
    leashed_ = state.leashed;
    armor_ = state.armor;
    updateVariantPresentation();
}

bool MobEntity::collides(const World& world, const glm::vec3& position) const
{
    const float halfWidth = collisionWidth() * 0.5f;
    const glm::vec3 minimum{
        position.x - halfWidth, position.y, position.z - halfWidth
    };
    const glm::vec3 maximum{
        position.x + halfWidth,
        position.y + collisionHeight(),
        position.z + halfWidth
    };
    for (int x = static_cast<int>(std::floor(minimum.x));
         x <= static_cast<int>(std::floor(maximum.x)); ++x)
    for (int y = static_cast<int>(std::floor(minimum.y));
         y <= static_cast<int>(std::floor(maximum.y)); ++y)
    for (int z = static_cast<int>(std::floor(minimum.z));
         z <= static_cast<int>(std::floor(maximum.z)); ++z)
    {
        const content::BlockState state = world.getActualBlockState(x, y, z);
        for (const BlockBox& box : getBlockShape(state).collisionBoxes)
            if (intersects(minimum, maximum, box, {x, y, z}))
                return true;
    }
    return false;
}

void MobEntity::moveAxis(const World& world, float amount, int axis)
{
    if (std::abs(amount) < 0.000001f)
        return;
    glm::vec3 candidate = position_;
    candidate[axis] += amount;
    if (!collides(world, candidate))
    {
        position_ = candidate;
        return;
    }
    if (axis == 1 && amount < 0.0f)
        onGround_ = true;
    velocity_[axis] = 0.0f;
}

void MobEntity::updateAgeableState()
{
    if (!definition_->ageable)
        return;
    if (growingAge_ < 0)
        ++growingAge_;
    else if (growingAge_ > 0)
        --growingAge_;
    if (forcedAgeTimer_ > 0)
        --forcedAgeTimer_;
}

void MobEntity::updateEnvironment(const World& world)
{
    const int x = static_cast<int>(std::floor(position_.x));
    const int z = static_cast<int>(std::floor(position_.z));
    const int feet = static_cast<int>(std::floor(position_.y + 0.1f));
    const int body = static_cast<int>(std::floor(
        position_.y + collisionHeight() * 0.6f
    ));
    const BlockType feetBlock = world.getBlock(x, feet, z);
    const BlockType bodyBlock = world.getBlock(x, body, z);
    inWater_ = feetBlock == BlockType::Water || bodyBlock == BlockType::Water;
    inLava_ = feetBlock == BlockType::Lava || bodyBlock == BlockType::Lava;
}

void MobEntity::applyMovement(const World& world)
{
    const bool flying = definition_->movementKind ==
        gameplay::MobMovementKind::Flying;
    const bool aquatic = definition_->movementKind ==
        gameplay::MobMovementKind::Aquatic;
    const bool liquid = inWater_ || inLava_;

    if (jumpRequested_)
    {
        if (liquid)
            velocity_.y += 0.04f;
        else if (onGround_)
            velocity_.y = 0.42f;
    }

    if (flying)
    {
        velocity_ *= 0.91f;
    }
    else if (aquatic || liquid)
    {
        if (!aquatic)
            velocity_.y -= 0.02f;
        const float drag = inLava_ ? 0.5f : 0.8f;
        velocity_.x *= drag;
        velocity_.y *= drag;
        velocity_.z *= drag;
    }
    else
    {
        velocity_.y -= 0.08f;
        velocity_.y *= 0.98f;
        const float friction = onGround_ ? 0.546f : 0.91f;
        velocity_.x *= friction;
        velocity_.z *= friction;
    }

    const bool wasOnGround = onGround_;
    onGround_ = false;
    const glm::vec3 beforeHorizontal = position_;
    const float desiredX = velocity_.x;
    const float desiredZ = velocity_.z;
    moveAxis(world, desiredX, 0);
    moveAxis(world, desiredZ, 2);
    const bool blockedHorizontally =
        std::abs(position_.x - beforeHorizontal.x - desiredX) > 0.0001f ||
        std::abs(position_.z - beforeHorizontal.z - desiredZ) > 0.0001f;
    if (blockedHorizontally && wasOnGround && definition_->stepHeight > 0.0f)
    {
        const glm::vec3 blocked = position_;
        position_ = beforeHorizontal;
        if (!collides(world, position_ + glm::vec3(
                0.0f, definition_->stepHeight, 0.0f)))
        {
            position_.y += definition_->stepHeight;
            velocity_.x = desiredX;
            velocity_.z = desiredZ;
            moveAxis(world, velocity_.x, 0);
            moveAxis(world, velocity_.z, 2);
        }
        else
        {
            position_ = blocked;
        }
    }
    moveAxis(world, velocity_.y, 1);
}

void MobEntity::updateAnimation() noexcept
{
    const glm::vec2 horizontalDelta(
        position_.x - previousPosition_.x,
        position_.z - previousPosition_.z
    );
    const float movement = std::min(
        1.0f,
        std::sqrt(glm::dot(horizontalDelta, horizontalDelta)) * 4.0f
    );
    limbSwingAmount_ += (movement - limbSwingAmount_) * 0.4f;
    limbSwing_ += limbSwingAmount_;
}

bool MobEntity::isBreedingItem(ItemType item) const noexcept
{
    const auto& foods = definition_->breedingItems;
    return std::find(foods.begin(), foods.end(), item) != foods.end();
}

bool MobEntity::isTamingItem(ItemType item) const noexcept
{
    const auto& foods = definition_->tamingItems;
    return std::find(foods.begin(), foods.end(), item) != foods.end();
}

bool MobEntity::canMateWith(const MobEntity& other) const noexcept
{
    if (&other == this || !isInLove() || !other.isInLove())
        return false;
    const std::string& name = type_.path();
    const std::string& otherName = other.type_.path();
    if (name == "horse" || name == "donkey")
    {
        if (otherName != "horse" && otherName != "donkey")
            return false;
        return tamed_ && other.tamed_ && !isChild() && !other.isChild() &&
            health_ >= definition_->maximumHealth &&
            other.health_ >= other.definition_->maximumHealth;
    }
    if (name == "llama")
    {
        return otherName == "llama" && tamed_ && other.tamed_ &&
            !isChild() && !other.isChild() &&
            health_ >= definition_->maximumHealth &&
            other.health_ >= other.definition_->maximumHealth;
    }
    if (name == "wolf" || name == "ocelot")
        return type_ == other.type_ && tamed_ && other.tamed_ &&
            !other.sitting_;
    return type_ == other.type_;
}

void MobEntity::consumeOne(ItemStack& stack) noexcept
{
    if (stack.empty())
        return;
    if (stack.count > 1)
        --stack.count;
    else
        stack.clear();
}

void MobEntity::updateVariantPresentation()
{
    texture_ = definition_->variantTextures.empty()
        ? definition_->texture
        : definition_->variantTextures[
            static_cast<std::size_t>(std::abs(variant_)) %
            definition_->variantTextures.size()
        ];
    overlayTexture_ = definition_->overlayTexture;
    if (!definition_->variantOverlayTextures.empty())
    {
        const std::size_t index = static_cast<std::size_t>(
            std::abs(variant_ / 17)
        ) % (definition_->variantOverlayTextures.size() + 1U);
        overlayTexture_ = index == 0U
            ? core::ResourceLocation("minecraft:entity/empty")
            : definition_->variantOverlayTextures[index - 1U];
    }
    overlayColour_ = {1.0f, 1.0f, 1.0f};
    if (type_.path() == "sheep")
    {
        constexpr std::array<std::uint32_t, 16> colours{{
            16383998U, 16351261U, 13061821U, 3847130U,
            16701501U, 8439583U, 15961002U, 4673362U,
            10329495U, 1481884U, 8991416U, 3949738U,
            8606770U, 6192150U, 11546150U, 1908001U
        }};
        const int metadata = std::clamp(variant_, 0, 15);
        if (metadata == 0)
        {
            overlayColour_ = {0.9019608f, 0.9019608f, 0.9019608f};
        }
        else
        {
            const std::uint32_t colour = colours[static_cast<std::size_t>(
                metadata
            )];
            overlayColour_ = {
                static_cast<float>((colour >> 16U) & 255U) / 255.0f * 0.75f,
                static_cast<float>((colour >> 8U) & 255U) / 255.0f * 0.75f,
                static_cast<float>(colour & 255U) / 255.0f * 0.75f
            };
        }
    }
}

void MobEntity::rebindRuntime() noexcept
{
    if (ai_)
        ai_->rebind(*this);
}
}
