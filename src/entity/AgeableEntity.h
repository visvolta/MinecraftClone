#pragma once

#include "entity/Creature.h"

namespace mc::entity
{
class AgeableEntity : public Creature
{
public:
    explicit AgeableEntity(World& world);

    void onLivingUpdate();
    [[nodiscard]] bool isChild() const;
    void setGrowingAge(int age);
    [[nodiscard]] int getGrowingAge() const noexcept { return growingAge_; }
    void addGrowth(int seconds);
    [[nodiscard]] float getRenderScale() const override;

protected:
    int growingAge_ = 0;
    int forcedAge_ = 0;
    int forcedAgeTimer_ = 0;
};
}
