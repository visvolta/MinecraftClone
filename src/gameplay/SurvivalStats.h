#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace mc::gameplay
{
enum class StatusEffectType : std::uint8_t
{
    Speed,
    Slowness,
    Haste,
    MiningFatigue,
    Strength,
    Regeneration,
    Resistance,
    FireResistance,
    WaterBreathing,
    Hunger,
    Weakness,
    Poison,
    JumpBoost,
    Levitation
};

struct StatusEffect
{
    StatusEffectType type = StatusEffectType::Speed;
    int durationTicks = 0;
    std::uint8_t amplifier = 0;
    bool ambient = false;
    bool showParticles = true;
};

struct SurvivalPersistentState
{
    int foodLevel = 20;
    float saturation = 5.0f;
    float exhaustion = 0.0f;
    int foodTickTimer = 0;
    int experienceLevel = 0;
    int experienceTotal = 0;
    float experienceProgress = 0.0f;
    int armorPoints = 0;
    float armorToughness = 0.0f;
};

// Release 1.12 food, experience, armor, effects, and attack-cooldown state.
// The class owns no rendering or input, so mods can reuse it for players and
// living entities without depending on the client executable.
class SurvivalStats
{
public:
    void tick(bool canHeal, bool canStarve, int& health, int maximumHealth);
    void addExhaustion(float amount) noexcept;
    void eat(int food, float saturationModifier) noexcept;
    void addExperience(int amount) noexcept;
    void resetAttackCooldown() noexcept;
    void setAttackSpeed(float attacksPerSecond) noexcept;
    void setArmor(int points, float toughness) noexcept;
    void addEffect(StatusEffect effect);
    void clearEffects() noexcept;

    [[nodiscard]] int foodLevel() const noexcept;
    [[nodiscard]] float saturation() const noexcept;
    [[nodiscard]] float exhaustion() const noexcept;
    [[nodiscard]] int experienceLevel() const noexcept;
    [[nodiscard]] int experienceTotal() const noexcept;
    [[nodiscard]] float experienceProgress() const noexcept;
    [[nodiscard]] int armorPoints() const noexcept;
    [[nodiscard]] float armorToughness() const noexcept;
    [[nodiscard]] float attackStrength(float partialTick = 0.0f) const noexcept;
    [[nodiscard]] bool hasEffect(StatusEffectType type) const noexcept;
    [[nodiscard]] int effectAmplifier(StatusEffectType type) const noexcept;
    [[nodiscard]] const std::vector<StatusEffect>& effects() const noexcept;

    [[nodiscard]] SurvivalPersistentState persistentState() const noexcept;
    void restorePersistentState(const SurvivalPersistentState& state) noexcept;
    void respawn() noexcept;

private:
    SurvivalPersistentState state_{};
    std::vector<StatusEffect> effects_;
    int attackTicker_ = 20;
    float attackSpeed_ = 4.0f;

    [[nodiscard]] int xpBarCapacity() const noexcept;
    void tickEffects(int& health, int maximumHealth);
};
}
