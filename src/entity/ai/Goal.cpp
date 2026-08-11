#include "entity/ai/Goal.h"

#include <algorithm>
#include <stdexcept>

namespace mc::entity::ai
{
bool Goal::shouldContinue(GoalContext& context)
{
    return shouldExecute(context);
}

void Goal::start(GoalContext&) {}
void Goal::reset(GoalContext&) {}
void Goal::tick(GoalContext&) {}
bool Goal::interruptible() const noexcept { return true; }

void Goal::setMutexBits(std::uint8_t bits) noexcept { mutexBits_ = bits; }
std::uint8_t Goal::mutexBits() const noexcept { return mutexBits_; }

Goal& GoalSelector::add(int priority, std::unique_ptr<Goal> goal)
{
    if (!goal)
        throw std::invalid_argument("Cannot register an empty entity goal");
    Goal& result = *goal;
    entries_.push_back({
        priority, nextInsertionOrder_++, std::move(goal), false
    });
    return result;
}

bool GoalSelector::remove(const Goal* goal, GoalContext& context)
{
    const auto found = std::find_if(
        entries_.begin(), entries_.end(),
        [goal](const Entry& entry) { return entry.action.get() == goal; }
    );
    if (found == entries_.end())
        return false;
    if (found->running)
        found->action->reset(context);
    entries_.erase(found);
    return true;
}

void GoalSelector::clear(GoalContext* context)
{
    if (context != nullptr)
    {
        for (Entry& entry : entries_)
            if (entry.running)
                entry.action->reset(*context);
    }
    entries_.clear();
    tickCount_ = 0;
}

void GoalSelector::tick(GoalContext& context)
{
    if (tickCount_++ % tickRate_ == 0)
    {
        for (Entry& entry : entries_)
        {
            if (entry.running)
            {
                if (!canUse(entry) ||
                    !entry.action->shouldContinue(context))
                {
                    entry.running = false;
                    entry.action->reset(context);
                }
            }
            else if (canUse(entry) && entry.action->shouldExecute(context))
            {
                entry.running = true;
                entry.action->start(context);
            }
        }
    }
    else
    {
        for (Entry& entry : entries_)
        {
            if (entry.running && !entry.action->shouldContinue(context))
            {
                entry.running = false;
                entry.action->reset(context);
            }
        }
    }

    for (Entry& entry : entries_)
        if (entry.running)
            entry.action->tick(context);
}

void GoalSelector::disableControlFlag(std::uint8_t flag) noexcept
{
    disabledControlFlags_ |= flag;
}

void GoalSelector::enableControlFlag(std::uint8_t flag) noexcept
{
    disabledControlFlags_ &= static_cast<std::uint8_t>(~flag);
}

void GoalSelector::setControlFlag(
    std::uint8_t flag,
    bool enabled) noexcept
{
    if (enabled)
        enableControlFlag(flag);
    else
        disableControlFlag(flag);
}

bool GoalSelector::isControlFlagDisabled(std::uint8_t flag) const noexcept
{
    return (disabledControlFlags_ & flag) != 0;
}

std::size_t GoalSelector::size() const noexcept { return entries_.size(); }

std::size_t GoalSelector::runningCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        entries_.begin(), entries_.end(),
        [](const Entry& entry) { return entry.running; }
    ));
}

bool GoalSelector::canUse(const Entry& candidate) const
{
    if (isControlFlagDisabled(candidate.action->mutexBits()))
        return false;

    for (const Entry& running : entries_)
    {
        if (!running.running || &running == &candidate)
            continue;
        if (candidate.priority >= running.priority)
        {
            if (!compatible(candidate, running))
                return false;
        }
        else if (!running.action->interruptible())
        {
            return false;
        }
    }
    return true;
}

bool GoalSelector::compatible(
    const Entry& first,
    const Entry& second) noexcept
{
    return (first.action->mutexBits() & second.action->mutexBits()) == 0;
}
}
