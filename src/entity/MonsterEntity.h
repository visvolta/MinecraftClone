#pragma once

#include "entity/Creature.h"

namespace mc::entity
{
class MonsterEntity : public Creature
{
public:
    explicit MonsterEntity(World& world);

    [[nodiscard]] EnumCreatureType getCreatureType() const override
    {
        return EnumCreatureType::Monster;
    }
    void onLivingUpdate() override;
    void onUpdate() override;
    [[nodiscard]] bool getCanSpawnHere() override;
    [[nodiscard]] float getBlockPathWeight(int x, int y, int z) const override;
    bool attackEntityAsMob(Entity& target) override;

protected:
    void applyEntityAttributes() override;
    [[nodiscard]] bool isValidLightLevel();
};
}
