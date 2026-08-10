#include "gameplay/SurvivalStats.h"

#include <cmath>

namespace mc::gameplay
{
void SurvivalStats::tick(
    bool canHeal,
    bool canStarve,
    int& health,
    int maximumHealth)
{
    ++attackTicker_;
    tickEffects(health, maximumHealth);

    if (state_.exhaustion > 4.0f)
    {
        state_.exhaustion -= 4.0f;
        if (state_.saturation > 0.0f)
            state_.saturation = std::max(0.0f, state_.saturation - 1.0f);
        else if (state_.foodLevel > 0)
            --state_.foodLevel;
    }

    if (canHeal && state_.foodLevel >= 18 && health < maximumHealth)
    {
        ++state_.foodTickTimer;
        // 1.12 natural regeneration: one point every four seconds and six
        // exhaustion. Fast saturation healing was introduced after 1.12.
        if (state_.foodTickTimer >= 80)
        {
            health = std::min(maximumHealth, health + 1);
            addExhaustion(6.0f);
            state_.foodTickTimer = 0;
        }
    }
    else if (canStarve && state_.foodLevel <= 0)
    {
        ++state_.foodTickTimer;
        if (state_.foodTickTimer >= 80)
        {
            if (health > 1)
                --health;
            state_.foodTickTimer = 0;
        }
    }
    else
    {
        state_.foodTickTimer = 0;
    }
}

void SurvivalStats::addExhaustion(float amount) noexcept
{
    state_.exhaustion = std::min(40.0f, state_.exhaustion + std::max(0.0f, amount));
}

void SurvivalStats::eat(int food, float saturationModifier) noexcept
{
    state_.foodLevel = std::clamp(state_.foodLevel + food, 0, 20);
    state_.saturation = std::clamp(
        state_.saturation + static_cast<float>(food) * saturationModifier * 2.0f,
        0.0f,
        static_cast<float>(state_.foodLevel)
    );
}

int SurvivalStats::xpBarCapacity() const noexcept
{
    if (state_.experienceLevel >= 30)
        return 112 + (state_.experienceLevel - 30) * 9;
    if (state_.experienceLevel >= 15)
        return 37 + (state_.experienceLevel - 15) * 5;
    return 7 + state_.experienceLevel * 2;
}

void SurvivalStats::addExperience(int amount) noexcept
{
    if (amount <= 0)
        return;
    state_.experienceTotal = std::max(0, state_.experienceTotal + amount);
    float progressPoints = state_.experienceProgress * xpBarCapacity() + amount;
    while (progressPoints >= static_cast<float>(xpBarCapacity()))
    {
        progressPoints -= static_cast<float>(xpBarCapacity());
        ++state_.experienceLevel;
    }
    state_.experienceProgress = progressPoints /
        static_cast<float>(std::max(1, xpBarCapacity()));
}

void SurvivalStats::resetAttackCooldown() noexcept
{
    attackTicker_ = 0;
}

void SurvivalStats::setAttackSpeed(float attacksPerSecond) noexcept
{
    attackSpeed_ = std::clamp(attacksPerSecond, 0.1f, 20.0f);
}

void SurvivalStats::setArmor(int points, float toughness) noexcept
{
    state_.armorPoints = std::clamp(points, 0, 20);
    state_.armorToughness = std::max(0.0f, toughness);
}

void SurvivalStats::addEffect(StatusEffect effect)
{
    if (effect.durationTicks <= 0)
        return;
    for (StatusEffect& current : effects_)
    {
        if (current.type != effect.type)
            continue;
        if (effect.amplifier > current.amplifier ||
            (effect.amplifier == current.amplifier &&
             effect.durationTicks > current.durationTicks))
        {
            current = effect;
        }
        return;
    }
    effects_.push_back(effect);
}

void SurvivalStats::clearEffects() noexcept
{
    effects_.clear();
}

void SurvivalStats::tickEffects(int& health, int maximumHealth)
{
    for (StatusEffect& effect : effects_)
    {
        const int interval = std::max(1, 50 >> effect.amplifier);
        if (effect.type == StatusEffectType::Regeneration &&
            effect.durationTicks % interval == 0 && health < maximumHealth)
        {
            ++health;
        }
        else if (effect.type == StatusEffectType::Poison &&
                 effect.durationTicks % interval == 0 && health > 1)
        {
            --health;
        }
        else if (effect.type == StatusEffectType::Hunger)
        {
            addExhaustion(0.005f * static_cast<float>(effect.amplifier + 1));
        }
        --effect.durationTicks;
    }
    std::erase_if(effects_, [](const StatusEffect& effect)
    {
        return effect.durationTicks <= 0;
    });
}

int SurvivalStats::foodLevel() const noexcept { return state_.foodLevel; }
float SurvivalStats::saturation() const noexcept { return state_.saturation; }
float SurvivalStats::exhaustion() const noexcept { return state_.exhaustion; }
int SurvivalStats::experienceLevel() const noexcept { return state_.experienceLevel; }
int SurvivalStats::experienceTotal() const noexcept { return state_.experienceTotal; }
float SurvivalStats::experienceProgress() const noexcept { return state_.experienceProgress; }
int SurvivalStats::armorPoints() const noexcept { return state_.armorPoints; }
float SurvivalStats::armorToughness() const noexcept { return state_.armorToughness; }

float SurvivalStats::attackStrength(float partialTick) const noexcept
{
    const float cooldownTicks = 20.0f / attackSpeed_;
    return std::clamp(
        (static_cast<float>(attackTicker_) + partialTick) / cooldownTicks,
        0.0f,
        1.0f
    );
}

bool SurvivalStats::hasEffect(StatusEffectType type) const noexcept
{
    return effectAmplifier(type) >= 0;
}

int SurvivalStats::effectAmplifier(StatusEffectType type) const noexcept
{
    for (const StatusEffect& effect : effects_)
    {
        if (effect.type == type)
            return effect.amplifier;
    }
    return -1;
}

const std::vector<StatusEffect>& SurvivalStats::effects() const noexcept
{
    return effects_;
}

SurvivalPersistentState SurvivalStats::persistentState() const noexcept
{
    return state_;
}

void SurvivalStats::restorePersistentState(
    const SurvivalPersistentState& state) noexcept
{
    state_ = state;
    state_.foodLevel = std::clamp(state_.foodLevel, 0, 20);
    state_.saturation = std::clamp(
        state_.saturation, 0.0f, static_cast<float>(state_.foodLevel)
    );
    state_.exhaustion = std::clamp(state_.exhaustion, 0.0f, 40.0f);
    state_.foodTickTimer = std::max(0, state_.foodTickTimer);
    state_.experienceLevel = std::max(0, state_.experienceLevel);
    state_.experienceTotal = std::max(0, state_.experienceTotal);
    state_.experienceProgress = std::clamp(state_.experienceProgress, 0.0f, 1.0f);
    state_.armorPoints = std::clamp(state_.armorPoints, 0, 20);
    state_.armorToughness = std::max(0.0f, state_.armorToughness);
    attackTicker_ = 20;
}

void SurvivalStats::respawn() noexcept
{
    state_ = {};
    effects_.clear();
    attackTicker_ = 20;
}
}
