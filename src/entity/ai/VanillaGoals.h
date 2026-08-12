#pragma once

#include "Item.h"
#include "entity/ai/Goal.h"

#include <glm/glm.hpp>

#include <functional>
#include <optional>
#include <vector>

namespace mc::entity
{
class Mob;
class LivingEntity;
class PlayerEntity;
class AnimalEntity;
class TameableEntity;
class Creature;
}

namespace mc::entity::ai
{
[[nodiscard]] std::optional<glm::dvec3> findRandomTarget(
    Mob& mob,
    int xz,
    int y,
    const glm::dvec3* direction = nullptr,
    bool avoidWater = false
);

class SwimGoal final : public Goal
{
public:
    explicit SwimGoal(Mob& mob);
    bool shouldExecute(GoalContext&) override;
    void tick(GoalContext&) override;

private:
    Mob* mob_ = nullptr;
};

class PanicGoal final : public Goal
{
public:
    PanicGoal(Mob& mob, double speed);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;

private:
    Mob* mob_ = nullptr;
    double speed_ = 1.0;
    double targetX_ = 0.0;
    double targetY_ = 0.0;
    double targetZ_ = 0.0;
};

class WanderAvoidWaterGoal final : public Goal
{
public:
    WanderAvoidWaterGoal(Mob& mob, double speed, float chance = 0.001f);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;

private:
    Mob* mob_ = nullptr;
    double speed_ = 1.0;
    float chance_ = 0.001f;
    double targetX_ = 0.0;
    double targetY_ = 0.0;
    double targetZ_ = 0.0;
};

class WatchClosestGoal final : public Goal
{
public:
    WatchClosestGoal(Mob& mob, float range, float chance = 0.02f);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;
    void tick(GoalContext&) override;

private:
    Mob* mob_ = nullptr;
    LivingEntity* closest_ = nullptr;
    float range_ = 8.0f;
    float chance_ = 0.02f;
    int lookTime_ = 0;
};

class LookIdleGoal final : public Goal
{
public:
    explicit LookIdleGoal(Mob& mob);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;
    void tick(GoalContext&) override;

private:
    Mob* mob_ = nullptr;
    double lookX_ = 0.0;
    double lookZ_ = 0.0;
    int idleTime_ = 0;
};

class AttackMeleeGoal final : public Goal
{
public:
    AttackMeleeGoal(Mob& mob, double speed, bool longMemory);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;
    void reset(GoalContext&) override;
    void tick(GoalContext&) override;

private:
    Mob* mob_ = nullptr;
    double speed_ = 1.0;
    bool longMemory_ = false;
    int delayCounter_ = 0;
    double targetX_ = 0.0;
    double targetY_ = 0.0;
    double targetZ_ = 0.0;
};

class NearestAttackableTargetGoal final : public Goal
{
public:
    NearestAttackableTargetGoal(
        Mob& mob,
        bool playersOnly,
        float range = 0.0f,
        bool checkSight = true
    );
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;

private:
    Mob* mob_ = nullptr;
    bool playersOnly_ = true;
    float range_ = 0.0f;
    bool checkSight_ = true;
    LivingEntity* target_ = nullptr;
};

class HurtByTargetGoal final : public Goal
{
public:
    explicit HurtByTargetGoal(Mob& mob, bool callForHelp = false);
    bool shouldExecute(GoalContext&) override;
    void start(GoalContext&) override;

private:
    Mob* mob_ = nullptr;
    bool callForHelp_ = false;
};

class MateGoal final : public Goal
{
public:
    MateGoal(AnimalEntity& animal, double speed);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;
    void tick(GoalContext&) override;

private:
    AnimalEntity* animal_ = nullptr;
    AnimalEntity* mate_ = nullptr;
    double speed_ = 1.0;
    int spawnBabyDelay_ = 0;
};

class TemptGoal final : public Goal
{
public:
    TemptGoal(
        Creature& creature,
        double speed,
        std::vector<ItemType> items,
        bool scaredByPlayerMovement
    );
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;
    void reset(GoalContext&) override;
    void tick(GoalContext&) override;

private:
    Creature* creature_ = nullptr;
    double speed_ = 1.0;
    std::vector<ItemType> items_;
    bool scared_ = false;
    int delayTemptCounter_ = 0;
    PlayerEntity* temptingPlayer_ = nullptr;
};

class FollowParentGoal final : public Goal
{
public:
    FollowParentGoal(AnimalEntity& animal, double speed);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void tick(GoalContext&) override;

private:
    AnimalEntity* animal_ = nullptr;
    AnimalEntity* parent_ = nullptr;
    double speed_ = 1.0;
    int delay_ = 0;
};

class EatGrassGoal final : public Goal
{
public:
    explicit EatGrassGoal(Mob& mob);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;
    void reset(GoalContext&) override;
    void tick(GoalContext&) override;
    [[nodiscard]] int getEatingGrassTimer() const noexcept { return timer_; }

private:
    Mob* mob_ = nullptr;
    int timer_ = 0;
};

class SitGoal final : public Goal
{
public:
    explicit SitGoal(TameableEntity& tameable);
    bool shouldExecute(GoalContext&) override;
    void start(GoalContext&) override;
    void reset(GoalContext&) override;

private:
    TameableEntity* tameable_ = nullptr;
};

class FollowOwnerGoal final : public Goal
{
public:
    FollowOwnerGoal(TameableEntity& tameable, double speed, float minDist, float maxDist);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;
    void reset(GoalContext&) override;
    void tick(GoalContext&) override;

private:
    TameableEntity* tameable_ = nullptr;
    double speed_ = 1.0;
    float minDist_ = 10.0f;
    float maxDist_ = 2.0f;
    int timeToRecalc_ = 0;
};

class LeapAtTargetGoal final : public Goal
{
public:
    LeapAtTargetGoal(Mob& mob, float leapMotionY);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;

private:
    Mob* mob_ = nullptr;
    float leapMotionY_ = 0.4f;
};

class AvoidEntityGoal final : public Goal
{
public:
    AvoidEntityGoal(
        Creature& creature,
        float avoidDistance,
        double farSpeed,
        double nearSpeed,
        std::function<bool(LivingEntity&)> selector
    );
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;
    void reset(GoalContext&) override;

private:
    Creature* creature_ = nullptr;
    float avoidDistance_ = 8.0f;
    double farSpeed_ = 1.0;
    double nearSpeed_ = 1.2;
    std::function<bool(LivingEntity&)> selector_;
    LivingEntity* closest_ = nullptr;
    double targetX_ = 0.0;
    double targetY_ = 0.0;
    double targetZ_ = 0.0;
};

class CreeperSwellGoal final : public Goal
{
public:
    explicit CreeperSwellGoal(Mob& creeper);
    bool shouldExecute(GoalContext&) override;
    void start(GoalContext&) override;
    void reset(GoalContext&) override;
    void tick(GoalContext&) override;

private:
    Mob* creeper_ = nullptr;
};

class AttackRangedBowGoal final : public Goal
{
public:
    AttackRangedBowGoal(Mob& mob, double speed, int attackCooldown, float maxDistance);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;
    void reset(GoalContext&) override;
    void tick(GoalContext&) override;

private:
    Mob* mob_ = nullptr;
    double speed_ = 1.0;
    int attackCooldown_ = 20;
    float maxAttackDistance_ = 15.0f;
    int seeTime_ = 0;
    int attackTime_ = -1;
    int strafingTime_ = -1;
    bool strafingClockwise_ = false;
    bool strafingBackwards_ = false;
};

class BegGoal final : public Goal
{
public:
    BegGoal(TameableEntity& wolf, float minDistance);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;
    void reset(GoalContext&) override;
    void tick(GoalContext&) override;

private:
    TameableEntity* wolf_ = nullptr;
    PlayerEntity* player_ = nullptr;
    float minDistance_ = 8.0f;
    int timeout_ = 0;
};

class RestrictSunGoal final : public Goal
{
public:
    explicit RestrictSunGoal(Creature& creature);
    bool shouldExecute(GoalContext&) override;
    void start(GoalContext&) override;
    void reset(GoalContext&) override;

private:
    Creature* creature_ = nullptr;
};

class FleeSunGoal final : public Goal
{
public:
    FleeSunGoal(Creature& creature, double movementSpeed);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;

private:
    Creature* creature_ = nullptr;
    double speed_ = 1.0;
    double shelterX_ = 0.0;
    double shelterY_ = 0.0;
    double shelterZ_ = 0.0;
};

class MoveTowardsRestrictionGoal final : public Goal
{
public:
    MoveTowardsRestrictionGoal(Creature& creature, double speed);
    bool shouldExecute(GoalContext&) override;
    bool shouldContinue(GoalContext&) override;
    void start(GoalContext&) override;

private:
    Creature* creature_ = nullptr;
    double speed_ = 1.0;
    double targetX_ = 0.0;
    double targetY_ = 0.0;
    double targetZ_ = 0.0;
};
}
