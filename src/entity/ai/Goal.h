#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace mc::entity::ai
{
class GoalContext
{
public:
    virtual ~GoalContext() = default;
};

// Port of EntityAIBase's lifecycle and control flags. 1.12 uses bit 1 for
// movement, bit 2 for looking and bit 4 for jumping.
class Goal
{
public:
    virtual ~Goal() = default;

    [[nodiscard]] virtual bool shouldExecute(GoalContext& context) = 0;
    [[nodiscard]] virtual bool shouldContinue(GoalContext& context);
    virtual void start(GoalContext& context);
    virtual void reset(GoalContext& context);
    virtual void tick(GoalContext& context);
    [[nodiscard]] virtual bool interruptible() const noexcept;

    void setMutexBits(std::uint8_t bits) noexcept;
    [[nodiscard]] std::uint8_t mutexBits() const noexcept;

private:
    std::uint8_t mutexBits_ = 0;
};

// EntityAITasks in 1.12 evaluates the complete task set every third tick and
// only rechecks running tasks on the two intervening ticks. Entries retain
// insertion order when priorities are equal.
class GoalSelector
{
public:
    Goal& add(int priority, std::unique_ptr<Goal> goal);
    bool remove(const Goal* goal, GoalContext& context);
    void clear(GoalContext* context = nullptr);
    void tick(GoalContext& context);

    void disableControlFlag(std::uint8_t flag) noexcept;
    void enableControlFlag(std::uint8_t flag) noexcept;
    void setControlFlag(std::uint8_t flag, bool enabled) noexcept;

    [[nodiscard]] bool isControlFlagDisabled(
        std::uint8_t flag
    ) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t runningCount() const noexcept;

private:
    struct Entry
    {
        int priority = 0;
        std::size_t insertionOrder = 0;
        std::unique_ptr<Goal> action;
        bool running = false;
    };

    std::vector<Entry> entries_;
    std::size_t nextInsertionOrder_ = 0;
    int tickCount_ = 0;
    int tickRate_ = 3;
    std::uint8_t disabledControlFlags_ = 0;

    [[nodiscard]] bool canUse(const Entry& candidate) const;
    [[nodiscard]] static bool compatible(
        const Entry& first,
        const Entry& second
    ) noexcept;
};
}
