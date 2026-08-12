#pragma once

#include <vector>

namespace mc::entity
{
class Entity;
class Mob;

class EntitySenses
{
public:
    explicit EntitySenses(Mob& owner);
    void clearSensingCache();
    [[nodiscard]] bool canSee(Entity& entity);

private:
    Mob* owner_ = nullptr;
    std::vector<Entity*> seen_;
    std::vector<Entity*> unseen_;
};
}
