#pragma once

#include "entity/AnimalEntity.h"

namespace mc::entity
{
class TameableEntity : public AnimalEntity
{
public:
    explicit TameableEntity(World& world);

    [[nodiscard]] bool isTamed() const noexcept { return tamed_; }
    virtual void setTamed(bool tamed);
    [[nodiscard]] bool isSitting() const override { return sitting_; }
    void setSitting(bool sitting) noexcept { sitting_ = sitting; }
    [[nodiscard]] const EntityUuid& getOwnerId() const noexcept { return ownerId_; }
    void setOwnerId(const EntityUuid& id) noexcept { ownerId_ = id; }
    [[nodiscard]] PlayerEntity* getOwner() const;
    [[nodiscard]] bool isOwner(const LivingEntity& entity) const;
    [[nodiscard]] bool canDespawn() const override { return !isTamed(); }

protected:
    bool tamed_ = false;
    bool sitting_ = false;
    EntityUuid ownerId_{};
};
}
