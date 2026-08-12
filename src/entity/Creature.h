#pragma once

#include "entity/Mob.h"

namespace mc::entity
{
class Creature : public Mob
{
public:
    explicit Creature(World& world);

    [[nodiscard]] float getBlockPathWeight(int x, int y, int z) const override;
    [[nodiscard]] bool getCanSpawnHere() override;
    [[nodiscard]] bool hasPath() const;
    void setHomePosAndDistance(int x, int y, int z, int distance);
    [[nodiscard]] bool isWithinHomeDistanceCurrentPosition() const;
    void detachHome();
    [[nodiscard]] bool hasHome() const noexcept { return maximumHomeDistance_ != -1.0f; }

protected:
    int homeX_ = 0;
    int homeY_ = 0;
    int homeZ_ = 0;
    float maximumHomeDistance_ = -1.0f;
};
}
