#pragma once

#include "entity/ai/Goal.h"
#include "entity/navigation/PathNavigation.h"

#include <glm/glm.hpp>

#include <optional>

namespace mc::gameplay { struct MobGoalDefinition; }

namespace mc::entity
{
class MobEntity;
struct MobTickContext;

namespace ai
{
class MobAiController
{
public:
    explicit MobAiController(MobEntity& owner);
    ~MobAiController();

    MobAiController(const MobAiController&) = delete;
    MobAiController& operator=(const MobAiController&) = delete;

    void tick(MobTickContext& context);
    void rebind(MobEntity& owner) noexcept;
    void clearPath() noexcept;
    [[nodiscard]] bool noPath() const noexcept;

    // Goal-facing API. Goal implementations remain independent objects while
    // the controller is the only class granted access to entity internals.
    [[nodiscard]] MobEntity& owner() noexcept;
    [[nodiscard]] const MobEntity& owner() const noexcept;
    [[nodiscard]] MobTickContext& context();
    [[nodiscard]] bool navigateTo(const glm::vec3& target, double speed);
    [[nodiscard]] bool navigateTo(
        const World& world,
        const glm::vec3& target,
        double speed
    );
    void lookAt(const glm::vec3& target, float yawLimit, float pitchLimit);
    [[nodiscard]] std::optional<glm::vec3> randomTarget(
        int horizontal,
        int vertical,
        const glm::vec3* direction = nullptr,
        bool avoidWater = false
    );
    [[nodiscard]] MobEntity* nearestSameType(
        float horizontalRange,
        float verticalRange,
        bool adultOnly,
        bool mateOnly
    );
    void queueChild(MobEntity& mate);
    void attackPlayer();
    void explode();
    void setPlayerTarget(bool targeted) noexcept;
    [[nodiscard]] bool playerTargeted() const noexcept;

private:
    class Context final : public GoalContext
    {
    public:
        explicit Context(MobAiController& controller) noexcept;
        MobAiController& controller;
    };

    MobEntity* owner_ = nullptr;
    MobTickContext* context_ = nullptr;
    Context goalContext_;
    GoalSelector tasks_;
    GoalSelector targetTasks_;
    navigation::PathNavigation navigator_;
    bool playerTargeted_ = false;

    void registerGoals();
    void addGoal(const gameplay::MobGoalDefinition& definition, bool target);
};
}
}
