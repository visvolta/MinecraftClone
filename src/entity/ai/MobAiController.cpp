#include "entity/ai/MobAiController.h"

#include "Block.h"
#include "Player.h"
#include "World.h"
#include "entity/MobEntity.h"
#include "gameplay/GameplayRegistries.h"
#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <utility>

namespace mc::entity::ai
{
namespace
{
constexpr float Pi = std::numbers::pi_v<float>;

bool containsItem(const std::vector<ItemType>& items, ItemType item)
{
    return std::find(items.begin(), items.end(), item) != items.end();
}

float distanceSquared(const glm::vec3& first, const glm::vec3& second)
{
    const glm::vec3 delta = second - first;
    return glm::dot(delta, delta);
}

glm::ivec3 floorPosition(const glm::vec3& position)
{
    return {
        static_cast<int>(std::floor(position.x)),
        static_cast<int>(std::floor(position.y)),
        static_cast<int>(std::floor(position.z))
    };
}

int mixSheepColours(int first, int second, JavaRandom& random)
{
    if (first == second)
        return first;
    const int low = std::min(first, second);
    const int high = std::max(first, second);
    if (low == 4 && high == 14) return 1;
    if (low == 6 && high == 10) return 2;
    if (low == 0 && high == 11) return 3;
    if (low == 0 && high == 13) return 5;
    if (low == 0 && high == 14) return 6;
    if (low == 0 && high == 15) return 7;
    if (low == 0 && high == 7) return 8;
    if (low == 11 && high == 13) return 9;
    if (low == 11 && high == 14) return 10;
    return random.nextBoolean() ? first : second;
}
}

class VanillaGoal final : public Goal
{
public:
    VanillaGoal(
        MobAiController& controller,
        gameplay::MobGoalDefinition definition)
        : controller_(controller), definition_(std::move(definition))
    {
        setMutexBits(definition_.mutexBits);
    }

    bool shouldExecute(GoalContext&) override
    {
        MobEntity& entity = controller_.owner();
        MobTickContext& context = controller_.context();
        const glm::vec3 playerPosition = context.player.getPosition();
        const float playerDistance = distanceSquared(
            entity.position_, playerPosition
        );

        using enum gameplay::MobGoalKind;
        switch (definition_.kind)
        {
            case Swim:
                return entity.inWater_ || entity.inLava_;
            case Panic:
                if (entity.panicTicks_ <= 0 && entity.fireTicks_ <= 0 &&
                    entity.revengeTicks_ <= 0)
                    return false;
                if (const auto target = controller_.randomTarget(5, 4))
                {
                    targetPosition_ = *target;
                    return true;
                }
                return false;
            case Mate:
                if (!entity.isInLove())
                    return false;
                targetMob_ = controller_.nearestSameType(8.0f, 4.0f,
                                                        false, true);
                return targetMob_ != nullptr;
            case Tempt:
                if (delay_ > 0)
                {
                    --delay_;
                    return false;
                }
                return context.player.isAlive() &&
                    playerDistance <= definition_.range * definition_.range &&
                    containsItem(definition_.items, context.playerMainHand);
            case FollowParent:
                if (!entity.isChild())
                    return false;
                targetMob_ = controller_.nearestSameType(
                    definition_.range, 4.0f, true, false
                );
                return targetMob_ != nullptr &&
                    distanceSquared(entity.position_, targetMob_->position_) >=
                        9.0f;
            case WanderAvoidWater:
                if (context.random.nextFloat() >= std::max(
                        definition_.chance, 1.0f / 120.0f))
                    return false;
                if (const auto target = controller_.randomTarget(
                        10, 7, nullptr, true))
                {
                    targetPosition_ = *target;
                    return true;
                }
                return false;
            case WatchPlayer:
                return context.player.isAlive() &&
                    context.random.nextFloat() < definition_.chance &&
                    playerDistance <= definition_.range * definition_.range;
            case LookIdle:
                return context.random.nextFloat() < definition_.chance;
            case EatGrass:
            {
                const int chance = entity.isChild() ? 50 : 1000;
                if (context.random.nextInt(chance) != 0)
                    return false;
                const glm::ivec3 position = floorPosition(entity.position_);
                return context.world.getBlock(
                           position.x, position.y, position.z) ==
                           BlockType::TallGrass ||
                       context.world.getBlock(
                           position.x, position.y - 1, position.z) ==
                           BlockType::Grass;
            }
            case AvoidPlayer:
            {
                if (!context.player.isAlive() ||
                    playerDistance > definition_.range * definition_.range)
                    return false;
                glm::vec3 away = entity.position_ - playerPosition;
                if (glm::dot(away, away) < 0.0001f)
                    away = {1.0f, 0.0f, 0.0f};
                if (const auto target = controller_.randomTarget(
                        16, 7, &away, true))
                {
                    if (distanceSquared(*target, playerPosition) <=
                        playerDistance)
                        return false;
                    targetPosition_ = *target;
                    return true;
                }
                return false;
            }
            case Sit:
                return entity.tamed_ && entity.sitting_ &&
                    !entity.inWater_ && !entity.inLava_ && entity.onGround_;
            case FollowOwner:
                return entity.tamed_ && !entity.sitting_ &&
                    entity.ownerUuid_ == context.player.uuid() &&
                    context.player.isAlive() &&
                    playerDistance >= definition_.range * definition_.range;
            case Beg:
                return context.player.isAlive() &&
                    playerDistance <= definition_.range * definition_.range &&
                    (context.playerMainHand == ItemType::Bone ||
                     entity.isBreedingItem(context.playerMainHand));
            case LeapAtTarget:
                return controller_.playerTargeted() && entity.onGround_ &&
                    playerDistance >= 4.0f && playerDistance <= 16.0f &&
                    context.random.nextInt(5) == 0;
            case MeleeAttack:
                return controller_.playerTargeted() &&
                    context.player.isAlive();
            case NearestPlayerTarget:
                return context.player.isAlive() &&
                    playerDistance <= definition_.range * definition_.range;
            case HurtByTarget:
                return entity.revengeTicks_ > 0 && context.player.isAlive();
            case RestrictSun:
                return context.daytime;
            case FleeSun:
            {
                if (!context.daytime)
                    return false;
                const glm::ivec3 eyes = floorPosition(
                    entity.position_ + glm::vec3(0.0f, entity.eyeHeight(), 0.0f)
                );
                if (context.world.getSkyLightLevel(
                        eyes.x, eyes.y, eyes.z) < 15)
                    return false;
                int bestLight = 15;
                bool found = false;
                for (int attempt = 0; attempt < 10; ++attempt)
                {
                    const glm::ivec3 candidate = floorPosition(entity.position_) +
                        glm::ivec3(
                            context.random.nextInt(20) - 10,
                            context.random.nextInt(6) - 3,
                            context.random.nextInt(20) - 10
                        );
                    const int light = context.world.getSkyLightLevel(
                        candidate.x, candidate.y, candidate.z
                    );
                    if (light < bestLight && context.world.getBlock(
                            candidate.x, candidate.y, candidate.z) ==
                            BlockType::Air)
                    {
                        bestLight = light;
                        targetPosition_ = glm::vec3(candidate) +
                            glm::vec3(0.5f, 0.0f, 0.5f);
                        found = true;
                    }
                }
                return found;
            }
            case CreeperSwell:
                return entity.fuseTicks_ > 0 ||
                    (controller_.playerTargeted() && playerDistance < 9.0f);
            case RangedAttack:
                return controller_.playerTargeted() &&
                    context.player.isAlive();
            case LandOnOwnerShoulder:
                return entity.tamed_ && !entity.sitting_ &&
                    entity.ownerUuid_ == context.player.uuid() &&
                    playerDistance < 1.0f && entity.onGround_;
            case RunAroundLikeCrazy:
                if (entity.tamed_ || !entity.beingRidden_)
                    return false;
                if (const auto target = controller_.randomTarget(5, 4))
                {
                    targetPosition_ = *target;
                    return true;
                }
                return false;
            case FollowCaravan:
            {
                if (entity.leashed_ || !entity.caravanHeadUuid_.empty())
                    return false;
                MobEntity* head = nullptr;
                float closest = std::numeric_limits<float>::max();
                for (MobEntity& candidate : context.entities)
                {
                    if (&candidate == &entity || candidate.dead() ||
                        candidate.type_.path() != "llama" ||
                        distanceSquared(entity.position_, candidate.position_) >
                            81.0f)
                        continue;
                    if (!candidate.caravanHeadUuid_.empty() &&
                        candidate.caravanTailUuid_.empty())
                    {
                        const float value = distanceSquared(
                            entity.position_, candidate.position_
                        );
                        if (value <= closest)
                        {
                            closest = value;
                            head = &candidate;
                        }
                    }
                }
                if (head == nullptr)
                {
                    for (MobEntity& candidate : context.entities)
                    {
                        if (&candidate == &entity || candidate.dead() ||
                            candidate.type_.path() != "llama" ||
                            !candidate.leashed_ ||
                            !candidate.caravanTailUuid_.empty())
                            continue;
                        const float value = distanceSquared(
                            entity.position_, candidate.position_
                        );
                        if (value <= 81.0f && value <= closest)
                        {
                            closest = value;
                            head = &candidate;
                        }
                    }
                }
                if (head == nullptr || closest < 4.0f ||
                    (!head->leashed_ && !firstIsLeashed(*head, 1)))
                    return false;
                entity.caravanHeadUuid_ = head->uuid_;
                head->caravanTailUuid_ = entity.uuid_;
                targetMob_ = head;
                speedModifier_ = definition_.speed;
                return true;
            }
        }
        return false;
    }

    bool shouldContinue(GoalContext&) override
    {
        MobEntity& entity = controller_.owner();
        MobTickContext& context = controller_.context();
        using enum gameplay::MobGoalKind;
        switch (definition_.kind)
        {
            case Swim: return entity.inWater_ || entity.inLava_;
            case Panic:
            case WanderAvoidWater:
            case AvoidPlayer:
            case FleeSun:
                return !controller_.noPath();
            case Mate:
                return targetMob_ != nullptr && !targetMob_->dead() &&
                    targetMob_->isInLove() && timer_ < 60;
            case Tempt:
                return context.player.isAlive() &&
                    containsItem(definition_.items,
                                 context.playerMainHand) &&
                    distanceSquared(entity.position_,
                                    context.player.getPosition()) <= 100.0f;
            case FollowParent:
            {
                if (!entity.isChild() || targetMob_ == nullptr ||
                    targetMob_->dead())
                    return false;
                const float value = distanceSquared(
                    entity.position_, targetMob_->position_
                );
                return value >= 9.0f && value <= 256.0f;
            }
            case WatchPlayer:
            case LookIdle:
            case Beg:
                return timer_ >= 0 && context.player.isAlive();
            case EatGrass: return timer_ > 0;
            case Sit:
                return entity.tamed_ && entity.sitting_ &&
                    entity.onGround_ && !entity.inWater_ && !entity.inLava_;
            case FollowOwner:
                return entity.tamed_ && !entity.sitting_ &&
                    !controller_.noPath() &&
                    distanceSquared(entity.position_,
                                    context.player.getPosition()) >
                        definition_.stopRange * definition_.stopRange;
            case LeapAtTarget: return !entity.onGround_;
            case MeleeAttack:
            case RangedAttack:
                return controller_.playerTargeted() &&
                    context.player.isAlive();
            case NearestPlayerTarget:
                return context.player.isAlive() &&
                    distanceSquared(entity.position_,
                                    context.player.getPosition()) <=
                        definition_.range * definition_.range;
            case HurtByTarget: return entity.revengeTicks_ > 0;
            case RestrictSun: return context.daytime;
            case CreeperSwell:
                return entity.fuseTicks_ > 0 ||
                    controller_.playerTargeted();
            case LandOnOwnerShoulder: return false;
            case RunAroundLikeCrazy:
                return !entity.tamed_ && entity.beingRidden_ &&
                    !controller_.noPath();
            case FollowCaravan:
            {
                targetMob_ = entityByUuid(entity.caravanHeadUuid_);
                if (targetMob_ == nullptr || targetMob_->dead() ||
                    !firstIsLeashed(entity, 0))
                    return false;
                const float value = distanceSquared(
                    entity.position_, targetMob_->position_
                );
                if (value > 676.0f)
                {
                    if (speedModifier_ <= 3.0)
                    {
                        speedModifier_ *= 1.2;
                        distCheckCounter_ = 40;
                        return true;
                    }
                    if (distCheckCounter_ == 0)
                        return false;
                }
                if (distCheckCounter_ > 0)
                    --distCheckCounter_;
                return true;
            }
        }
        return false;
    }

    void start(GoalContext&) override
    {
        MobEntity& entity = controller_.owner();
        MobTickContext& context = controller_.context();
        using enum gameplay::MobGoalKind;
        switch (definition_.kind)
        {
            case Panic:
            case WanderAvoidWater:
            case AvoidPlayer:
            case FleeSun:
                static_cast<void>(controller_.navigateTo(
                    targetPosition_, definition_.speed
                ));
                break;
            case Mate:
                timer_ = 0;
                break;
            case Tempt:
                targetPosition_ = context.player.getPosition();
                previousPlayerPosition_ = targetPosition_;
                previousPlayerLook_ = context.player.getLookDirection();
                break;
            case FollowParent:
            case FollowOwner:
                timer_ = 0;
                break;
            case WatchPlayer:
            case Beg:
                timer_ = 40 + context.random.nextInt(40);
                if (definition_.kind == Beg)
                    entity.begging_ = true;
                break;
            case LookIdle:
            {
                const double angle = std::numbers::pi_v<double> * 2.0 *
                    context.random.nextDouble();
                lookDirection_ = {
                    static_cast<float>(std::cos(angle)), 0.0f,
                    static_cast<float>(std::sin(angle))
                };
                timer_ = 20 + context.random.nextInt(20);
                break;
            }
            case EatGrass:
                timer_ = 40;
                entity.eatTicks_ = 40;
                controller_.clearPath();
                break;
            case Sit:
                controller_.clearPath();
                break;
            case LeapAtTarget:
            {
                glm::vec3 direction = context.player.getPosition() -
                    entity.position_;
                direction.y = 0.0f;
                const float length = std::sqrt(glm::dot(direction, direction));
                if (length >= 0.0001f)
                {
                    direction /= length;
                    entity.velocity_.x += direction.x * 0.5f *
                        0.800000011920929f + entity.velocity_.x *
                        0.20000000298023224f;
                    entity.velocity_.z += direction.z * 0.5f *
                        0.800000011920929f + entity.velocity_.z *
                        0.20000000298023224f;
                }
                entity.velocity_.y = definition_.range;
                break;
            }
            case MeleeAttack:
            case RangedAttack:
                timer_ = 0;
                static_cast<void>(controller_.navigateTo(
                    context.player.getPosition(), definition_.speed
                ));
                break;
            case NearestPlayerTarget:
            case HurtByTarget:
                controller_.setPlayerTarget(true);
                break;
            case CreeperSwell:
                controller_.clearPath();
                break;
            case LandOnOwnerShoulder:
                entity.sitting_ = true;
                controller_.clearPath();
                break;
            case Swim:
            case RestrictSun:
                break;
            case RunAroundLikeCrazy:
                static_cast<void>(controller_.navigateTo(
                    targetPosition_, definition_.speed
                ));
                break;
            case FollowCaravan: break;
        }
    }

    void reset(GoalContext&) override
    {
        MobEntity& entity = controller_.owner();
        using enum gameplay::MobGoalKind;
        switch (definition_.kind)
        {
            case Mate:
            case FollowParent:
                targetMob_ = nullptr;
                timer_ = 0;
                break;
            case Tempt:
                controller_.clearPath();
                delay_ = 100;
                break;
            case Beg:
                entity.begging_ = false;
                break;
            case NearestPlayerTarget:
            case HurtByTarget:
                controller_.setPlayerTarget(false);
                break;
            case CreeperSwell:
                if (entity.fuseTicks_ > 0)
                    --entity.fuseTicks_;
                break;
            case FollowCaravan:
            {
                MobEntity* head = entityByUuid(entity.caravanHeadUuid_);
                if (head != nullptr && head->caravanTailUuid_ == entity.uuid_)
                    head->caravanTailUuid_ = {};
                entity.caravanHeadUuid_ = {};
                targetMob_ = nullptr;
                speedModifier_ = 2.1;
                break;
            }
            default: break;
        }
    }

    void tick(GoalContext&) override
    {
        MobEntity& entity = controller_.owner();
        MobTickContext& context = controller_.context();
        const glm::vec3 playerPosition = context.player.getPosition();
        using enum gameplay::MobGoalKind;
        switch (definition_.kind)
        {
            case Swim:
                if (context.random.nextFloat() < 0.8f)
                    entity.jumpRequested_ = true;
                break;
            case Mate:
                if (targetMob_ == nullptr)
                    break;
                controller_.lookAt(
                    targetMob_->position_ +
                        glm::vec3(0.0f, targetMob_->eyeHeight(), 0.0f),
                    10.0f, 40.0f
                );
                static_cast<void>(controller_.navigateTo(
                    targetMob_->position_, definition_.speed
                ));
                ++timer_;
                if (timer_ >= 60 && distanceSquared(
                        entity.position_, targetMob_->position_) < 9.0f)
                    controller_.queueChild(*targetMob_);
                break;
            case Tempt:
            {
                if (entity.type_.path() == "ocelot" &&
                    distanceSquared(entity.position_, playerPosition) < 36.0f)
                {
                    if (distanceSquared(previousPlayerPosition_,
                                        playerPosition) > 0.01f ||
                        glm::dot(previousPlayerLook_,
                                 context.player.getLookDirection()) <
                            std::cos(5.0f * Pi / 180.0f))
                    {
                        controller_.clearPath();
                        delay_ = 100;
                        return;
                    }
                }
                previousPlayerPosition_ = playerPosition;
                previousPlayerLook_ = context.player.getLookDirection();
                controller_.lookAt(
                    context.player.getEyePosition(), 30.0f, 40.0f
                );
                if (distanceSquared(entity.position_, playerPosition) < 6.25f)
                    controller_.clearPath();
                else
                    static_cast<void>(controller_.navigateTo(
                        playerPosition, definition_.speed
                    ));
                break;
            }
            case FollowParent:
                if (targetMob_ != nullptr && --timer_ <= 0)
                {
                    timer_ = 10;
                    static_cast<void>(controller_.navigateTo(
                        targetMob_->position_, definition_.speed
                    ));
                }
                break;
            case WatchPlayer:
            case Beg:
                controller_.lookAt(
                    context.player.getEyePosition(), 10.0f, 40.0f
                );
                --timer_;
                break;
            case LookIdle:
                --timer_;
                controller_.lookAt(
                    entity.position_ + lookDirection_ +
                        glm::vec3(0.0f, entity.eyeHeight(), 0.0f),
                    10.0f, 40.0f
                );
                break;
            case EatGrass:
            {
                timer_ = std::max(0, timer_ - 1);
                entity.eatTicks_ = timer_;
                if (timer_ != 4)
                    break;
                const glm::ivec3 position = floorPosition(entity.position_);
                if (context.world.getBlock(
                        position.x, position.y, position.z) ==
                    BlockType::TallGrass)
                {
                    context.world.setBlock(
                        position.x, position.y, position.z, BlockType::Air
                    );
                }
                else if (context.world.getBlock(
                             position.x, position.y - 1, position.z) ==
                         BlockType::Grass)
                {
                    context.world.setBlock(
                        position.x, position.y - 1, position.z,
                        BlockType::Dirt
                    );
                }
                entity.sheared_ = false;
                if (entity.isChild())
                    entity.growingAge_ = std::min(
                        0, entity.growingAge_ + 60 * 20
                    );
                break;
            }
            case FollowOwner:
                controller_.lookAt(
                    context.player.getEyePosition(), 10.0f, 40.0f
                );
                if (--timer_ <= 0)
                {
                    timer_ = 10;
                    const bool found = controller_.navigateTo(
                        playerPosition, definition_.speed
                    );
                    if (!found && distanceSquared(
                            entity.position_, playerPosition) >= 144.0f)
                    {
                        const glm::ivec3 owner = floorPosition(playerPosition);
                        for (int x = 0; x <= 4; ++x)
                        for (int z = 0; z <= 4; ++z)
                        {
                            if (x >= 1 && x <= 3 && z >= 1 && z <= 3)
                                continue;
                            const glm::ivec3 destination = owner +
                                glm::ivec3(x - 2, 0, z - 2);
                            if (!context.world.isSolidBlock(
                                    destination.x, destination.y - 1,
                                    destination.z) ||
                                context.world.isSolidBlock(
                                    destination.x, destination.y,
                                    destination.z) ||
                                context.world.isSolidBlock(
                                    destination.x, destination.y + 1,
                                    destination.z))
                                continue;
                            const glm::vec3 candidate =
                                glm::vec3(destination) +
                                glm::vec3(0.5f, 0.0f, 0.5f);
                            if (!entity.collides(context.world, candidate))
                            {
                                entity.position_ = candidate;
                                entity.previousPosition_ = candidate;
                                controller_.clearPath();
                                return;
                            }
                        }
                    }
                }
                break;
            case MeleeAttack:
            {
                controller_.lookAt(
                    context.player.getEyePosition(), 30.0f, 30.0f
                );
                if (--timer_ <= 0)
                {
                    timer_ = 4 + context.random.nextInt(7);
                    const float value = distanceSquared(
                        entity.position_, playerPosition
                    );
                    if (value > 1024.0f)
                        timer_ += 10;
                    else if (value > 256.0f)
                        timer_ += 5;
                    if (!controller_.navigateTo(
                            playerPosition, definition_.speed))
                        timer_ += 15;
                }
                if (entity.attackCooldown_ > 0)
                    --entity.attackCooldown_;
                const float reach = entity.collisionWidth() * 2.0f;
                const float reachSquared = reach * reach + 0.6f;
                if (distanceSquared(entity.position_, playerPosition) <=
                        reachSquared && entity.attackCooldown_ <= 0)
                {
                    entity.attackCooldown_ = 20;
                    controller_.attackPlayer();
                }
                break;
            }
            case LeapAtTarget:
                break;
            case RangedAttack:
                if (--timer_ <= 0 && distanceSquared(
                        entity.position_, playerPosition) <=
                        definition_.range * definition_.range)
                {
                    timer_ = std::max(1, entity.definition_->attackIntervalTicks);
                    controller_.attackPlayer();
                }
                break;
            case CreeperSwell:
            {
                const float value = distanceSquared(
                    entity.position_, playerPosition
                );
                if (controller_.playerTargeted() && value <= 49.0f)
                    ++entity.fuseTicks_;
                else
                    entity.fuseTicks_ = std::max(0, entity.fuseTicks_ - 1);
                if (entity.fuseTicks_ >= 30)
                    controller_.explode();
                break;
            }
            case NearestPlayerTarget:
            case HurtByTarget:
                controller_.setPlayerTarget(true);
                break;
            case Panic:
            case WanderAvoidWater:
            case AvoidPlayer:
            case Sit:
            case RestrictSun:
            case FleeSun:
            case LandOnOwnerShoulder:
                break;
            case RunAroundLikeCrazy:
                if (!entity.tamed_ && context.random.nextInt(50) == 0)
                {
                    if (entity.definition_->maximumTemper > 0 &&
                        context.random.nextInt(
                            entity.definition_->maximumTemper
                        ) < entity.temper_)
                    {
                        entity.tamed_ = true;
                        entity.ownerUuid_ = entity.riderUuid_;
                    }
                    else
                    {
                        entity.temper_ = std::min(
                            entity.definition_->maximumTemper,
                            entity.temper_ + 5
                        );
                        entity.beingRidden_ = false;
                        entity.riderUuid_ = {};
                        context.player.stopRiding();
                    }
                }
                break;
            case FollowCaravan:
            {
                targetMob_ = entityByUuid(entity.caravanHeadUuid_);
                if (targetMob_ == nullptr)
                    break;
                const glm::vec3 delta = targetMob_->position_ -
                    entity.position_;
                const float distance = std::sqrt(glm::dot(delta, delta));
                if (distance > 0.0001f)
                {
                    const glm::vec3 destination = entity.position_ +
                        delta / distance * std::max(distance - 2.0f, 0.0f);
                    static_cast<void>(controller_.navigateTo(
                        destination, speedModifier_
                    ));
                }
                break;
            }
        }
    }

private:
    MobAiController& controller_;
    gameplay::MobGoalDefinition definition_;
    MobEntity* targetMob_ = nullptr;
    glm::vec3 targetPosition_{};
    glm::vec3 previousPlayerPosition_{};
    glm::vec3 previousPlayerLook_{};
    glm::vec3 lookDirection_{};
    int timer_ = 0;
    int delay_ = 0;
    int distCheckCounter_ = 0;
    double speedModifier_ = 2.1;

    MobEntity* entityByUuid(const EntityUuid& uuid)
    {
        if (uuid.empty())
            return nullptr;
        for (MobEntity& candidate : controller_.context().entities)
            if (candidate.uuid_ == uuid)
                return &candidate;
        return nullptr;
    }

    bool firstIsLeashed(MobEntity& llama, int depth)
    {
        if (depth > 8 || llama.caravanHeadUuid_.empty())
            return false;
        MobEntity* head = entityByUuid(llama.caravanHeadUuid_);
        if (head == nullptr)
            return false;
        return head->leashed_ || firstIsLeashed(*head, depth + 1);
    }
};

MobAiController::Context::Context(MobAiController& value) noexcept
    : controller(value) {}

MobAiController::MobAiController(MobEntity& owner)
    : owner_(&owner), goalContext_(*this), navigator_([&owner]
      {
          navigation::NavigationSettings settings;
          settings.width = owner.definition_->width;
          settings.height = owner.definition_->height;
          settings.stepHeight = owner.definition_->stepHeight;
          settings.maximumFallHeight = owner.definition_->maximumFallHeight;
          settings.canSwim = gameplay::hasAiGoal(
              owner.definition_->aiGoals, gameplay::MobAiGoal::Swim
          );
          if (owner.type_.path() == "spider" ||
              owner.type_.path() == "cave_spider")
              settings.kind = navigation::NavigationKind::Climbing;
          else if (owner.definition_->movementKind ==
                   gameplay::MobMovementKind::Aquatic)
              settings.kind = navigation::NavigationKind::Swimming;
          else if (owner.definition_->movementKind ==
                   gameplay::MobMovementKind::Flying)
              settings.kind = navigation::NavigationKind::Flying;
          if (owner.type_.path() == "chicken")
              settings.setPriority(navigation::PathNodeType::Water, 0.0f);
          if (owner.type_.path() == "blaze")
          {
              settings.setPriority(navigation::PathNodeType::Water, -1.0f);
              settings.setPriority(navigation::PathNodeType::Lava, 8.0f);
              settings.setPriority(
                  navigation::PathNodeType::DangerFire, 0.0f
              );
              settings.setPriority(
                  navigation::PathNodeType::DamageFire, 0.0f
              );
          }
          if (owner.type_.path() == "enderman")
              settings.setPriority(navigation::PathNodeType::Water, -1.0f);
          return settings;
      }())
{
    registerGoals();
}

MobAiController::~MobAiController() = default;

void MobAiController::rebind(MobEntity& owner) noexcept
{
    owner_ = &owner;
}

void MobAiController::tick(MobTickContext& context)
{
    context_ = &context;
    playerTargeted_ = false;
    targetTasks_.tick(goalContext_);
    tasks_.tick(goalContext_);

    navigator_.settings().width = owner_->collisionWidth();
    navigator_.settings().height = owner_->collisionHeight();
    navigation::WorldNavigationBlockAccess access(context.world);
    navigator_.tick(
        access, owner_->position_, owner_->onGround_,
        owner_->inWater_ || owner_->inLava_
    );
    const glm::vec3 direction = navigator_.desiredDirection();
    owner_->movementSpeedMultiplier_ = static_cast<float>(navigator_.speed());
    if (glm::dot(direction, direction) > 0.0001f)
    {
        owner_->yaw_ = std::atan2(-direction.x, direction.z);
        const float acceleration =
            owner_->definition_->movementKind ==
                    gameplay::MobMovementKind::Aquatic
                ? 0.02f
                : 0.1f;
        owner_->velocity_.x += direction.x * owner_->definition_->movementSpeed *
            owner_->movementSpeedMultiplier_ * acceleration;
        owner_->velocity_.z += direction.z * owner_->definition_->movementSpeed *
            owner_->movementSpeedMultiplier_ * acceleration;
    }
    owner_->aggressive_ = playerTargeted_;
    context_ = nullptr;
}

void MobAiController::clearPath() noexcept { navigator_.clear(); }
bool MobAiController::noPath() const noexcept { return navigator_.noPath(); }
MobEntity& MobAiController::owner() noexcept { return *owner_; }
const MobEntity& MobAiController::owner() const noexcept { return *owner_; }

MobTickContext& MobAiController::context()
{
    return *context_;
}

bool MobAiController::navigateTo(const glm::vec3& target, double speed)
{
    return navigateTo(context_->world, target, speed);
}

bool MobAiController::navigateTo(
    const World& world,
    const glm::vec3& target,
    double speed)
{
    navigation::WorldNavigationBlockAccess access(world);
    return navigator_.tryMoveTo(
        access, owner_->position_, target, speed,
        std::max(16.0f, owner_->definition_->followRange)
    );
}

void MobAiController::lookAt(
    const glm::vec3& target,
    float yawLimit,
    float pitchLimit)
{
    const glm::vec3 eyes = owner_->position_ +
        glm::vec3(0.0f, owner_->eyeHeight(), 0.0f);
    const glm::vec3 delta = target - eyes;
    const float wantedYaw = std::atan2(-delta.x, delta.z) - owner_->yaw_;
    const float wantedPitch = -std::atan2(
        delta.y, std::sqrt(delta.x * delta.x + delta.z * delta.z)
    );
    const float yawRadians = yawLimit * Pi / 180.0f;
    const float pitchRadians = pitchLimit * Pi / 180.0f;
    owner_->headYaw_ = std::clamp(
        std::remainder(wantedYaw, Pi * 2.0f), -yawRadians, yawRadians
    );
    owner_->headPitch_ = std::clamp(
        wantedPitch, -pitchRadians, pitchRadians
    );
}

std::optional<glm::vec3> MobAiController::randomTarget(
    int horizontal,
    int vertical,
    const glm::vec3* direction,
    bool avoidWater)
{
    float bestWeight = -99999.0f;
    std::optional<glm::vec3> result;
    for (int attempt = 0; attempt < 10; ++attempt)
    {
        const int offsetX = context_->random.nextInt(horizontal * 2 + 1) -
            horizontal;
        const int offsetY = context_->random.nextInt(vertical * 2 + 1) -
            vertical;
        const int offsetZ = context_->random.nextInt(horizontal * 2 + 1) -
            horizontal;
        if (direction != nullptr &&
            static_cast<float>(offsetX) * direction->x +
            static_cast<float>(offsetZ) * direction->z < 0.0f)
            continue;
        glm::ivec3 candidate = floorPosition(owner_->position_) +
            glm::ivec3(offsetX, offsetY, offsetZ);
        if (!context_->world.isBlockLoaded(
                candidate.x, candidate.y, candidate.z))
            continue;
        while (candidate.y < Chunk::HEIGHT - 2 &&
               context_->world.isSolidBlock(
                   candidate.x, candidate.y, candidate.z))
            ++candidate.y;
        if (avoidWater && context_->world.getBlock(
                candidate.x, candidate.y, candidate.z) == BlockType::Water)
            continue;
        if (context_->world.isSolidBlock(
                candidate.x, candidate.y, candidate.z) ||
            context_->world.isSolidBlock(
                candidate.x, candidate.y + 1, candidate.z) ||
            !context_->world.isSolidBlock(
                candidate.x, candidate.y - 1, candidate.z))
            continue;
        float weight = static_cast<float>(
            context_->world.getSkyLightLevel(
                candidate.x, candidate.y, candidate.z
            )
        ) / 15.0f - 0.5f;
        if (owner_->definition_->animalKind != gameplay::AnimalKind::None &&
            context_->world.getBlock(
                candidate.x, candidate.y - 1, candidate.z) == BlockType::Grass)
            weight = 10.0f;
        if (weight > bestWeight)
        {
            bestWeight = weight;
            result = glm::vec3(candidate) + glm::vec3(0.5f, 0.0f, 0.5f);
        }
    }
    return result;
}

MobEntity* MobAiController::nearestSameType(
    float horizontalRange,
    float verticalRange,
    bool adultOnly,
    bool mateOnly)
{
    MobEntity* result = nullptr;
    float bestDistance = std::numeric_limits<float>::max();
    for (MobEntity& candidate : context_->entities)
    {
        if (&candidate == owner_ || candidate.dead())
            continue;
        if (mateOnly)
        {
            if (!owner_->canMateWith(candidate))
                continue;
        }
        else if (candidate.type_ != owner_->type_)
        {
            continue;
        }
        const glm::vec3 delta = candidate.position_ - owner_->position_;
        if (std::abs(delta.x) > horizontalRange ||
            std::abs(delta.z) > horizontalRange ||
            std::abs(delta.y) > verticalRange)
            continue;
        if (adultOnly && candidate.isChild())
            continue;
        const float value = glm::dot(delta, delta);
        if (value < bestDistance)
        {
            bestDistance = value;
            result = &candidate;
        }
    }
    return result;
}

void MobAiController::queueChild(MobEntity& mate)
{
    MobPersistentState child;
    child.type = owner_->type_;
    if ((owner_->type_.path() == "horse" &&
         mate.type_.path() == "donkey") ||
        (owner_->type_.path() == "donkey" &&
         mate.type_.path() == "horse"))
        child.type = core::ResourceLocation("minecraft:mule");
    child.uuid = EntityUuid::random();
    child.position = owner_->position_;
    child.yaw = 0.0f;
    child.growingAge = -24000;
    child.health = owner_->definition_->maximumHealth;

    if ((owner_->type_.path() == "wolf" ||
         owner_->type_.path() == "ocelot") && owner_->tamed_)
    {
        child.tamed = true;
        child.ownerUuid = owner_->ownerUuid_;
    }

    if (owner_->type_.path() == "sheep")
    {
        child.variant = mixSheepColours(
            owner_->variant_, mate.variant_, context_->random
        );
    }
    else if (owner_->type_.path() == "rabbit")
    {
        if (context_->random.nextInt(20) != 0)
            child.variant = context_->random.nextBoolean()
                ? owner_->variant_ : mate.variant_;
        else
            child.variant = context_->random.nextInt(6);
    }
    else if (owner_->type_.path() == "horse")
    {
        const int colourRoll = context_->random.nextInt(9);
        const int colour = colourRoll < 4 ? owner_->variant_ % 7
            : colourRoll < 8 ? mate.variant_ % 7
            : context_->random.nextInt(7);
        const int markingRoll = context_->random.nextInt(5);
        const int marking = markingRoll < 2 ? owner_->variant_ / 7
            : markingRoll < 4 ? mate.variant_ / 7
            : context_->random.nextInt(5);
        child.variant = colour + marking * 7;
    }
    else if (owner_->type_.path() == "llama")
    {
        child.variant = context_->random.nextBoolean()
            ? owner_->variant_ : mate.variant_;
    }
    else if (owner_->type_.path() == "ocelot")
    {
        child.variant = owner_->variant_;
    }
    else
    {
        child.variant = context_->random.nextBoolean()
            ? owner_->variant_ : mate.variant_;
    }

    owner_->growingAge_ = 6000;
    mate.growingAge_ = 6000;
    owner_->inLove_ = 0;
    mate.inLove_ = 0;
    owner_->loveCauseUuid_ = {};
    mate.loveCauseUuid_ = {};
    context_->player.addExperience(1 + context_->random.nextInt(7));
    context_->births.push_back({std::move(child)});
}

void MobAiController::attackPlayer()
{
    const int damage = std::max(
        1, static_cast<int>(std::floor(owner_->definition_->attackDamage))
    );
    context_->player.damage(damage, owner_->position_);
}

void MobAiController::explode()
{
    context_->player.damage(18, owner_->position_);
    constexpr int radius = 3;
    const glm::ivec3 centre = floorPosition(owner_->position_);
    for (int x = -radius; x <= radius; ++x)
    for (int y = -radius; y <= radius; ++y)
    for (int z = -radius; z <= radius; ++z)
    {
        if (x * x + y * y + z * z > radius * radius)
            continue;
        const glm::ivec3 block = centre + glm::ivec3(x, y, z);
        const BlockType type = context_->world.getBlock(
            block.x, block.y, block.z
        );
        if (type != BlockType::Air && type != BlockType::Bedrock &&
            type != BlockType::Obsidian)
            context_->world.setBlock(
                block.x, block.y, block.z, BlockType::Air
            );
    }
    owner_->health_ = 0.0f;
}

void MobAiController::setPlayerTarget(bool targeted) noexcept
{
    playerTargeted_ = playerTargeted_ || targeted;
}

bool MobAiController::playerTargeted() const noexcept
{
    return playerTargeted_;
}

void MobAiController::registerGoals()
{
    for (const gameplay::MobGoalDefinition& definition :
         owner_->definition_->goalTasks)
        addGoal(definition, false);
    for (const gameplay::MobGoalDefinition& definition :
         owner_->definition_->targetGoalTasks)
        addGoal(definition, true);
}

void MobAiController::addGoal(
    const gameplay::MobGoalDefinition& definition,
    bool target)
{
    auto goal = std::make_unique<VanillaGoal>(*this, definition);
    (target ? targetTasks_ : tasks_).add(
        definition.priority, std::move(goal)
    );
}
}
