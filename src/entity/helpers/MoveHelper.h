#pragma once

namespace mc::entity
{
class Mob;

class MoveHelper
{
public:
    enum class Action
    {
        Wait,
        MoveTo,
        Strafe,
        Jumping
    };

    explicit MoveHelper(Mob& owner);

    void setMoveTo(double x, double y, double z, double speed);
    void strafe(float forward, float strafe);
    void onUpdateMoveHelper();
    [[nodiscard]] bool isUpdating() const noexcept;
    [[nodiscard]] double getSpeed() const noexcept { return speed_; }

private:
    Mob* owner_ = nullptr;
    double posX_ = 0.0;
    double posY_ = 0.0;
    double posZ_ = 0.0;
    double speed_ = 0.0;
    float moveForward_ = 0.0f;
    float moveStrafe_ = 0.0f;
    Action action_ = Action::Wait;

    static float limitAngle(float source, float target, float maximum);
};
}
