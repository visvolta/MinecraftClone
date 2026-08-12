#pragma once

#include "entity/AgeableEntity.h"

namespace mc::entity
{
class AnimalEntity : public AgeableEntity
{
public:
    explicit AnimalEntity(World& world);

    [[nodiscard]] EnumCreatureType getCreatureType() const override
    {
        return EnumCreatureType::Creature;
    }
    [[nodiscard]] bool getCanSpawnHere() override;
    [[nodiscard]] float getBlockPathWeight(int x, int y, int z) const;
    [[nodiscard]] virtual bool isBreedingItem(ItemType item) const;
    bool processInteract(PlayerEntity& player, ItemStack& stack);
    void setInLove(PlayerEntity* player);
    [[nodiscard]] bool isInLove() const noexcept { return inLove_ > 0; }
    void resetInLove() noexcept { inLove_ = 0; }
    [[nodiscard]] bool canMateWith(AnimalEntity& other) const;
    virtual void spawnChildFromBreeding(AnimalEntity& mate);
    [[nodiscard]] virtual std::unique_ptr<AgeableEntity> createChild(
        AnimalEntity& mate) = 0;

    void onLivingUpdate();

protected:
    int inLove_ = 0;
    EntityUuid loveCause_{};
};
}
