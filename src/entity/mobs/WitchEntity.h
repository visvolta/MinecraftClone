#pragma once

#include "entity/MonsterEntity.h"
#include "entity/EntityUuid.h"

namespace mc::entity
{
class WitchEntity : public MonsterEntity
{
public:
    explicit WitchEntity(World& world);
    bool attackEntityAsMob(Entity& target) override;
    void onLivingUpdate() override;
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] core::ResourceLocation getLootTable() const override;
    [[nodiscard]] gameplay::MobModelKind getModelKind() const override;
    [[nodiscard]] core::ResourceLocation getTexture() const override;
    [[nodiscard]] float getEyeHeight() const override { return 1.62f; }
    [[nodiscard]] bool isDrinkingPotion() const noexcept { return drinking_; }

protected:
    void initEntityAI() override;
    void applyEntityAttributes() override;
    float applyPotionDamageCalculations(
        const DamageSource& source,
        float damage) override;

private:
    enum class DrinkPotion
    {
        None,
        WaterBreathing,
        FireResistance,
        Healing,
        Swiftness
    };

    static const EntityUuid DrinkModifierId;
    bool drinking_ = false;
    int potionUseTimer_ = 0;
    DrinkPotion pendingDrink_ = DrinkPotion::None;

    void setDrinkingPotion(bool drinking);
    void finishDrinking();
};
}
