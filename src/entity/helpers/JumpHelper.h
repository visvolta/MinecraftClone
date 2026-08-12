#pragma once

namespace mc::entity
{
class Mob;

class JumpHelper
{
public:
    explicit JumpHelper(Mob& owner);
    void setJumping();
    void doJump();

private:
    Mob* owner_ = nullptr;
    bool jumping_ = false;
};
}
