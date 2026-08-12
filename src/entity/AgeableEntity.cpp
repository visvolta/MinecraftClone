#include "entity/AgeableEntity.h"

namespace mc::entity
{
AgeableEntity::AgeableEntity(World& world) : Creature(world) {}

void AgeableEntity::onLivingUpdate()
{
    Creature::onLivingUpdate();
    const int age = growingAge_;
    if (age < 0)
    {
        setGrowingAge(age + 1);
        if (growingAge_ == 0)
            growingAge_ = forcedAge_;
    }
    else if (age > 0)
        setGrowingAge(age - 1);
    if (forcedAgeTimer_ > 0)
        --forcedAgeTimer_;
}

bool AgeableEntity::isChild() const
{
    return growingAge_ < 0;
}

void AgeableEntity::setGrowingAge(int age)
{
    const bool wasChild = isChild();
    growingAge_ = age;
    if (wasChild != isChild())
    {
        if (isChild())
            setSize(width_ * 0.5f, height_ * 0.5f);
        else
            setSize(width_ * 2.0f, height_ * 2.0f);
    }
}

void AgeableEntity::addGrowth(int seconds)
{
    int age = growingAge_;
    age += seconds * 20;
    if (age > 0)
        age = 0;
    const int old = growingAge_;
    setGrowingAge(age);
    forcedAge_ += growingAge_ - old;
    if (forcedAgeTimer_ == 0)
        forcedAgeTimer_ = 40;
}

float AgeableEntity::getRenderScale() const
{
    return isChild() ? 0.5f : 1.0f;
}
}
