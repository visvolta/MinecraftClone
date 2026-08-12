#pragma once

namespace mc::entity
{
class Mob;

class BodyHelper
{
public:
    explicit BodyHelper(Mob& owner);
    void updateRenderAngles();

private:
    Mob* owner_ = nullptr;
    int rotationTick_ = 0;
    float prevRenderYawHead_ = 0.0f;
};
}
