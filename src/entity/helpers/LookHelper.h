#pragma once

namespace mc::entity
{
class Entity;
class Mob;

class LookHelper
{
public:
    explicit LookHelper(Mob& owner);

    void setLookPositionWithEntity(Entity& entity, float deltaYaw, float deltaPitch);
    void setLookPosition(double x, double y, double z, float deltaYaw, float deltaPitch);
    void onUpdateLook();
    [[nodiscard]] bool getIsLooking() const noexcept { return looking_; }

private:
    Mob* owner_ = nullptr;
    float deltaLookYaw_ = 0.0f;
    float deltaLookPitch_ = 0.0f;
    bool looking_ = false;
    double posX_ = 0.0;
    double posY_ = 0.0;
    double posZ_ = 0.0;

    static float updateRotation(float current, float target, float maxChange);
};
}
