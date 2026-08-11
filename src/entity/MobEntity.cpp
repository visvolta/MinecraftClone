#include "entity/MobEntity.h"

#include "BlockShape.h"
#include "BlockProperties.h"
#include "Player.h"
#include "World.h"
#include "gameplay/GameplayRegistries.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numbers>

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
    texture_ = definition.variantTextures.empty()
        ? definition.texture
        : definition.variantTextures[
            variantSeed % definition.variantTextures.size()
        ];
    overlayTexture_ = definition.overlayTexture;
    if (!definition.variantOverlayTextures.empty())
    {
        const std::size_t overlayIndex =
            (variantSeed / 17U) % (definition.variantOverlayTextures.size()+1U);
        overlayTexture_ = overlayIndex == 0U
            ? core::ResourceLocation("minecraft:entity/empty")
            : definition.variantOverlayTextures[overlayIndex-1U];
    }
    if (type_.path() == "sheep")
    {
        const std::size_t colourRoll = variantSeed % 1000U;
        if (colourRoll < 50U) overlayColour_ = {0.10f,0.10f,0.10f};
        else if (colourRoll < 100U) overlayColour_ = {0.30f,0.30f,0.30f};
        else if (colourRoll < 150U) overlayColour_ = {0.60f,0.60f,0.60f};
        else if (colourRoll < 180U) overlayColour_ = {0.45f,0.30f,0.20f};
        else if (colourRoll < 182U) overlayColour_ = {0.95f,0.50f,0.65f};
    }
}

void MobEntity::tick(
    World& world,
    Player& player,
    bool daytime,
    std::mt19937& random)
{
    if (dead())
    {
        previousPosition_ = position_;
        previousYaw_ = yaw_;
        ++deathTicks_;
        return;
    }
    previousPosition_ = position_;
    previousYaw_ = yaw_;
    previousLimbSwing_ = limbSwing_;
    previousLimbSwingAmount_ = limbSwingAmount_;
    ++age_;
    if (attackCooldown_ > 0)
        --attackCooldown_;
    if (hurtTicks_ > 0)
        --hurtTicks_;
    if (panicTicks_ > 0)
        --panicTicks_;
    if (revengeTicks_ > 0)
        --revengeTicks_;
    if (eatTicks_ > 0)
    {
        --eatTicks_;
        if (eatTicks_ == 4)
        {
            const glm::ivec3 below(
                static_cast<int>(std::floor(position_.x)),
                static_cast<int>(std::floor(position_.y))-1,
                static_cast<int>(std::floor(position_.z))
            );
            if (world.getBlock(below.x,below.y,below.z)==BlockType::Grass)
                world.setBlock(below.x,below.y,below.z,BlockType::Dirt);
        }
    }
    else if (gameplay::hasAiGoal(
                 definition_->aiGoals, gameplay::MobAiGoal::EatGrass) &&
             onGround_ &&
             std::uniform_int_distribution<int>(0,999)(random)==0)
        eatTicks_=40;

    const glm::vec3 toPlayer = player.getPosition() - position_;
    const float horizontalDistanceSquared =
        toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z;
    const float distanceSquared = glm::dot(toPlayer, toPlayer);
    const bool attacksPlayer = gameplay::hasAiGoal(
        definition_->aiGoals, gameplay::MobAiGoal::AttackPlayer
    );
    aggressive_ = (attacksPlayer || revengeTicks_ > 0) && player.isAlive() &&
        distanceSquared < definition_->followRange * definition_->followRange;
    if (type_.path() == "enderman" && revengeTicks_ == 0 && player.isAlive())
    {
        const glm::vec3 playerToEyes =
            position_ + glm::vec3(0,definition_->eyeHeight,0) -
            player.getEyePosition();
        const float distance = std::sqrt(std::max(
            0.0001f, glm::dot(playerToEyes,playerToEyes)
        ));
        aggressive_ = glm::dot(
            glm::normalize(player.getLookDirection()),
            playerToEyes/distance
        ) > 1.0f - 0.025f/distance;
    }
    if ((type_.path() == "spider" || type_.path() == "cave_spider") &&
        daytime && revengeTicks_ == 0)
        aggressive_ = false;
    glm::vec2 direction(0.0f);
    if (aggressive_)
    {
        direction = glm::vec2(toPlayer.x, toPlayer.z);
    }
    else if (gameplay::hasAiGoal(
                 definition_->aiGoals, gameplay::MobAiGoal::AvoidPlayer) &&
             horizontalDistanceSquared < 8.0f * 8.0f)
    {
        direction = -glm::vec2(toPlayer.x, toPlayer.z);
    }
    else
    {
        if (--wanderTicks_ <= 0)
        {
            std::uniform_real_distribution<float> angle(
                0.0f, std::numbers::pi_v<float> * 2.0f
            );
            std::uniform_int_distribution<int> duration(40, 100);
            const float value = angle(random);
            wanderDirection_ = {std::cos(value), std::sin(value)};
            wanderTicks_ = duration(random);
        }
        direction = wanderDirection_;
    }
    if (panicTicks_ > 0)
        direction = -glm::vec2(toPlayer.x, toPlayer.z);
    if (!aggressive_ && daytime && gameplay::hasAiGoal(
            definition_->aiGoals, gameplay::MobAiGoal::AvoidSun) &&
        world.getSkyLightLevel(
            static_cast<int>(std::floor(position_.x)),
            static_cast<int>(std::floor(position_.y+definition_->eyeHeight)),
            static_cast<int>(std::floor(position_.z))) == 15)
    {
        int bestLight = 15;
        glm::vec2 shelterDirection(0.0f);
        std::uniform_int_distribution<int> shelterOffset(-10,10);
        for (int attempt=0; attempt<10; ++attempt)
        {
            const int targetX=static_cast<int>(std::floor(position_.x))+
                shelterOffset(random);
            const int targetZ=static_cast<int>(std::floor(position_.z))+
                shelterOffset(random);
            const int light=world.getSkyLightLevel(
                targetX,static_cast<int>(std::floor(position_.y)),targetZ
            );
            if(light<bestLight)
            {
                bestLight=light;
                shelterDirection={targetX-position_.x,targetZ-position_.z};
            }
        }
        if(glm::dot(shelterDirection,shelterDirection)>0.001f)
            direction=shelterDirection;
    }
    if (glm::dot(direction, direction) > 0.0001f)
    {
        direction = glm::normalize(direction);
        yaw_ = std::atan2(-direction.x, direction.y);
    }
    headYaw_ = std::clamp(
        std::atan2(-toPlayer.x, toPlayer.z) - yaw_,
        -1.309f, 1.309f
    );
    headPitch_ = -std::atan2(
        toPlayer.y - definition_->eyeHeight,
        std::sqrt(std::max(0.0001f, horizontalDistanceSquared))
    );

    const BlockType feetBlock = world.getBlock(
        static_cast<int>(std::floor(position_.x)),
        static_cast<int>(std::floor(position_.y + 0.1f)),
        static_cast<int>(std::floor(position_.z))
    );
    inWater_ = feetBlock == BlockType::Water;
    const bool flying = definition_->movementKind ==
        gameplay::MobMovementKind::Flying;
    const bool aquatic = definition_->movementKind ==
        gameplay::MobMovementKind::Aquatic;
    const bool hopping = definition_->movementKind ==
        gameplay::MobMovementKind::Hopping;
    const float acceleration = definition_->movementSpeed *
        (flying || aquatic || inWater_ ? 0.10f : 0.65f) *
        (panicTicks_ > 0 ? 1.5f : 1.0f);
    velocity_.x += direction.x * acceleration;
    velocity_.z += direction.y * acceleration;
    if (aggressive_ && onGround_ && gameplay::hasAiGoal(
            definition_->aiGoals, gameplay::MobAiGoal::LeapAtTarget) &&
        horizontalDistanceSquared > 4.0f &&
        horizontalDistanceSquared < 16.0f &&
        std::uniform_int_distribution<int>(0,4)(random) == 0)
    {
        velocity_.x += direction.x * 0.4f;
        velocity_.y = 0.4f;
        velocity_.z += direction.y * 0.4f;
    }

    if (flying)
    {
        velocity_.y += std::clamp(toPlayer.y, -1.0f, 1.0f) * 0.01f;
        velocity_ *= 0.91f;
    }
    else if (aquatic)
    {
        velocity_.y += std::clamp(toPlayer.y, -1.0f, 1.0f) *
            (aggressive_ ? 0.01f : 0.002f);
        if (!inWater_)
            velocity_.y -= 0.08f;
        velocity_ *= 0.80f;
    }
    else if (inWater_)
    {
        velocity_.y += 0.02f;
        velocity_ *= 0.80f;
    }
    else
    {
        velocity_.y -= 0.08f;
        velocity_.x *= onGround_ ? 0.60f : 0.91f;
        velocity_.y *= 0.98f;
        velocity_.z *= onGround_ ? 0.60f : 0.91f;
        if (hopping && onGround_ && age_ %
                (aggressive_ ? 10 : 20) == 0)
            velocity_.y = definition_->model == gameplay::MobModelKind::Rabbit
                ? 0.42f : 0.42f;
        else if (onGround_ && collides(
                world,
                position_ + glm::vec3(direction.x * 0.35f, 0.05f,
                                      direction.y * 0.35f)))
            velocity_.y = 0.42f;
    }

    onGround_ = false;
    moveAxis(world, velocity_.x, 0);
    moveAxis(world, velocity_.y, 1);
    moveAxis(world, velocity_.z, 2);

    const glm::vec2 horizontalDelta(
        position_.x - previousPosition_.x,
        position_.z - previousPosition_.z
    );
    const float movement = std::min(
        1.0f, std::sqrt(glm::dot(horizontalDelta, horizontalDelta)) * 4.0f
    );
    limbSwingAmount_ += (movement - limbSwingAmount_) * 0.4f;
    limbSwing_ += limbSwingAmount_;

    if (definition_->burnsInDaylight && daytime &&
        world.getSkyLightLevel(
            static_cast<int>(std::floor(position_.x)),
            static_cast<int>(std::floor(position_.y + definition_->eyeHeight)),
            static_cast<int>(std::floor(position_.z))) == 15)
    {
        fireTicks_ = std::max(fireTicks_, 160);
    }
    if (fireTicks_ > 0)
    {
        --fireTicks_;
        if (fireTicks_ % 20 == 0)
            damage(1.0f);
    }

    const float reach = std::max(
        definition_->attackRange,
        0.6f + definition_->width * 0.5f
    );
    if (aggressive_ && attackCooldown_ == 0 &&
        distanceSquared <= reach * reach)
    {
        switch (definition_->attackKind)
        {
            case gameplay::MobAttackKind::Melee:
                player.damage(std::max(
                    1, static_cast<int>(std::round(definition_->attackDamage))
                ), position_);
                attackCooldown_ = definition_->attackIntervalTicks;
                break;
            case gameplay::MobAttackKind::Ranged:
                player.damage(std::max(
                    1, static_cast<int>(std::round(
                        std::max(2.0f, definition_->attackDamage)
                    ))
                ), position_);
                attackCooldown_ = definition_->attackIntervalTicks;
                break;
            case gameplay::MobAttackKind::CreeperExplosion:
                ++fuseTicks_;
                if (fuseTicks_ >= definition_->attackIntervalTicks)
                {
                    player.damage(18, position_);
                    constexpr int radius=3;
                    const glm::ivec3 centre(glm::floor(position_));
                    for(int x=-radius;x<=radius;++x)
                    for(int y=-radius;y<=radius;++y)
                    for(int z=-radius;z<=radius;++z)
                    {
                        if(x*x+y*y+z*z>radius*radius)
                            continue;
                        const glm::ivec3 blockPosition=centre+
                            glm::ivec3(x,y,z);
                        const BlockType block=world.getBlock(
                            blockPosition.x,blockPosition.y,blockPosition.z
                        );
                        const BlockProperties& properties=
                            getBlockProperties(block);
                        if(block!=BlockType::Air && properties.breakable &&
                           properties.hardness<50.0f)
                            world.setBlock(
                                blockPosition.x,blockPosition.y,
                                blockPosition.z,BlockType::Air
                            );
                    }
                    health_ = 0.0f;
                }
                break;
            case gameplay::MobAttackKind::None: break;
        }
    }
    else if (definition_->attackKind ==
                 gameplay::MobAttackKind::CreeperExplosion && fuseTicks_ > 0)
        fuseTicks_ = std::max(0, fuseTicks_ - 1);
}

void MobEntity::damage(float amount, bool causedByPlayer) noexcept
{
    health_ = std::max(0.0f, health_ - std::max(0.0f, amount));
    hurtTicks_ = 10;
    if (definition_ != nullptr && gameplay::hasAiGoal(
            definition_->aiGoals, gameplay::MobAiGoal::Panic))
        panicTicks_ = 100;
    if (causedByPlayer && health_ <= 0.0f)
        killedByPlayer_ = true;
    if (causedByPlayer && definition_ != nullptr && gameplay::hasAiGoal(
            definition_->aiGoals, gameplay::MobAiGoal::HurtByTarget))
        revengeTicks_ = 400;
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
    return previousPosition_ + (position_ - previousPosition_) *
        std::clamp(partialTick, 0.0f, 1.0f);
}
float MobEntity::yaw() const noexcept { return yaw_; }
float MobEntity::interpolatedYaw(float partialTick) const noexcept
{
    const float delta=std::remainder(
        yaw_-previousYaw_,std::numbers::pi_v<float>*2.0f
    );
    return previousYaw_+delta*std::clamp(partialTick,0.0f,1.0f);
}
float MobEntity::health() const noexcept { return health_; }
int MobEntity::age() const noexcept { return age_; }
bool MobEntity::dead() const noexcept { return health_ <= 0.0f; }
bool MobEntity::killedByPlayer() const noexcept { return killedByPlayer_; }
int MobEntity::deathTicks() const noexcept { return deathTicks_; }

MobAnimationState MobEntity::animationState(float partialTick) const noexcept
{
    const float partial = std::clamp(partialTick, 0.0f, 1.0f);
    const float combatProgress = definition_->attackIntervalTicks > 0 &&
            attackCooldown_ > 0
        ? 1.0f - static_cast<float>(attackCooldown_) /
            static_cast<float>(definition_->attackIntervalTicks)
        : definition_->attackKind == gameplay::MobAttackKind::CreeperExplosion
            ? static_cast<float>(fuseTicks_) /
                static_cast<float>(std::max(1,definition_->attackIntervalTicks))
            : 0.0f;
    const float attackProgress = eatTicks_ > 0
        ? 1.0f - std::abs(static_cast<float>(eatTicks_-20))/20.0f
        : combatProgress;
    return {
        static_cast<float>(age_) + partial,
        previousLimbSwing_ + (limbSwing_ - previousLimbSwing_) * partial,
        previousLimbSwingAmount_ +
            (limbSwingAmount_ - previousLimbSwingAmount_) * partial,
        headYaw_, headPitch_, std::clamp(attackProgress,0.0f,1.0f),
        onGround_ ? 0.0f : std::clamp(std::abs(velocity_.y)*2.0f,0.0f,1.0f),
        static_cast<float>(hurtTicks_) / 10.0f,
        std::clamp(
            (static_cast<float>(deathTicks_) + partial) / 20.0f,
            0.0f, 1.0f
        ),
        onGround_, inWater_, aggressive_
    };
}

bool MobEntity::collides(const World& world, const glm::vec3& position) const
{
    const float halfWidth = definition_->width * 0.5f;
    const glm::vec3 minimum{
        position.x - halfWidth, position.y, position.z - halfWidth
    };
    const glm::vec3 maximum{
        position.x + halfWidth,
        position.y + definition_->height,
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
}
