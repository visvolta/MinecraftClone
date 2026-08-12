#pragma once

#include <algorithm>
#include <cmath>

namespace mc::entity
{
// net.minecraft.util.CombatRules
struct CombatRules
{
    [[nodiscard]] static float getDamageAfterAbsorb(
        float damage,
        float totalArmor,
        float toughnessAttribute) noexcept
    {
        const float f = 2.0f + toughnessAttribute / 4.0f;
        const float absorbed = std::clamp(
            totalArmor - damage / f,
            totalArmor * 0.2f,
            20.0f
        );
        return damage * (1.0f - absorbed / 25.0f);
    }

    [[nodiscard]] static float getDamageAfterMagicAbsorb(
        float damage,
        float enchantModifiers) noexcept
    {
        const float f = std::clamp(enchantModifiers, 0.0f, 20.0f);
        return damage * (1.0f - f / 25.0f);
    }
};
}
