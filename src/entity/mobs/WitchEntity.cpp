#include "entity/mobs/WitchEntity.h"

#include "World.h"
#include "entity/ai/VanillaGoals.h"
#include "entity/attributes/AttributeModifier.h"
#include "entity/projectile/ArrowEntity.h"

#include <cmath>

namespace mc::entity
{
const EntityUuid WitchEntity::DrinkModifierId{0x5CD17E52A79A43D3ULL, 0xA52990FDE04B181EULL};

WitchEntity::WitchEntity(World& world) : MonsterEntity(world)
{
    setSize(0.6f, 1.95f);
    experienceValue_ = 5;
}

void WitchEntity::initEntityAI()
{
    tasks_.add(1, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(2, std::make_unique<ai::AttackRangedBowGoal>(*this, 1.0, 60, 10.0f));
    tasks_.add(2, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 1.0));
    tasks_.add(3, std::make_unique<ai::WatchClosestGoal>(*this, 8.0f));
    tasks_.add(3, std::make_unique<ai::LookIdleGoal>(*this));
    targetTasks_.add(1, std::make_unique<ai::HurtByTargetGoal>(*this, false));
    targetTasks_.add(2, std::make_unique<ai::NearestAttackableTargetGoal>(*this, true));
}

void WitchEntity::applyEntityAttributes()
{
    MonsterEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(26.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.25);
}

void WitchEntity::setDrinkingPotion(bool drinking)
{
    drinking_ = drinking;
    AttributeInstance& speed =
        getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED);
    speed.removeModifier(DrinkModifierId);
    if (drinking)
    {
        speed.applyModifier(AttributeModifier(
            DrinkModifierId,
            "Drinking speed penalty",
            -0.25,
            AttributeModifier::Operation::Add,
            false));
    }
}

void WitchEntity::onLivingUpdate()
{
    if (drinking_)
    {
        if (--potionUseTimer_ <= 0)
        {
            setDrinkingPotion(false);
            if (const auto held = getHeldItemMainhand(); !held.empty())
            {
                // Effects were recorded when drinking started.
            }
            getHeldItemMainhand() = {};
        }
    }
    else
    {
        bool drink = false;
        if (rand_.nextFloat() < 0.15f && isInWater() &&
            !isPotionActive(gameplay::StatusEffectType::WaterBreathing))
        {
            addPotionEffect({gameplay::StatusEffectType::WaterBreathing, 3600, 0});
            drink = true;
        }
        else if (rand_.nextFloat() < 0.15f && isBurning() &&
                 !isPotionActive(gameplay::StatusEffectType::FireResistance))
        {
            addPotionEffect({gameplay::StatusEffectType::FireResistance, 3600, 0});
            drink = true;
        }
        else if (rand_.nextFloat() < 0.05f && getHealth() < getMaxHealth())
        {
            heal(4.0f);
            drink = true;
        }
        else if (rand_.nextFloat() < 0.5f && getAttackTarget() != nullptr &&
                 !isPotionActive(gameplay::StatusEffectType::Speed) &&
                 getAttackTarget()->getDistanceSq(*this) > 121.0)
        {
            addPotionEffect({gameplay::StatusEffectType::Speed, 3600, 0});
            drink = true;
        }
        if (drink)
        {
            potionUseTimer_ = 32;
            setDrinkingPotion(true);
        }
    }

    MonsterEntity::onLivingUpdate();
}

void WitchEntity::finishDrinking()
{
    setDrinkingPotion(false);
    switch (pendingDrink_)
    {
        case DrinkPotion::WaterBreathing:
            addPotionEffect({gameplay::StatusEffectType::WaterBreathing, 3600, 0});
            break;
        case DrinkPotion::FireResistance:
            addPotionEffect({gameplay::StatusEffectType::FireResistance, 3600, 0});
            break;
        case DrinkPotion::Healing:
            heal(4.0f);
            break;
        case DrinkPotion::Swiftness:
            addPotionEffect({gameplay::StatusEffectType::Speed, 3600, 0});
            break;
        case DrinkPotion::None:
            break;
    }
    pendingDrink_ = DrinkPotion::None;
}

float WitchEntity::applyPotionDamageCalculations(
    const DamageSource& source,
    float damage)
{
    damage = LivingEntity::applyPotionDamageCalculations(source, damage);
    if (source.getTrueSource() == this)
        damage = 0.0f;
    if (source.isMagicDamage())
        damage = static_cast<float>(static_cast<double>(damage) * 0.15);
    return damage;
}

bool WitchEntity::attackEntityAsMob(Entity& target)
{
    if (drinking_)
        return false;
    auto* living = dynamic_cast<LivingEntity*>(&target);
    if (!living)
        return false;

    const double dx = living->posX + living->motionX - posX;
    const double dz = living->posZ + living->motionZ - posZ;
    const float distance = static_cast<float>(std::sqrt(dx * dx + dz * dz));
    SplashPotionType type = SplashPotionType::Harming;
    if (distance >= 8.0f &&
        !living->isPotionActive(gameplay::StatusEffectType::Slowness))
        type = SplashPotionType::Slowness;
    else if (living->getHealth() >= 8.0f &&
             !living->isPotionActive(gameplay::StatusEffectType::Poison))
        type = SplashPotionType::Poison;
    else if (distance <= 3.0f &&
             !living->isPotionActive(gameplay::StatusEffectType::Weakness) &&
             rand_.nextFloat() < 0.25f)
        type = SplashPotionType::Weakness;

    auto potion = std::make_unique<ThrownPotionEntity>(getWorld(), this, type);
    const double dy =
        living->posY + living->getEyeHeight() - 1.100000023841858 - potion->posY;
    potion->rotationPitch -= -20.0f;
    potion->shoot(dx, dy + distance * 0.2, dz, 0.75f, 8.0f);
    getWorld().spawnEntity(std::move(potion));
    return true;
}

core::ResourceLocation WitchEntity::getType() const
{
    return core::ResourceLocation("minecraft:witch");
}
core::ResourceLocation WitchEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/witch");
}
gameplay::MobModelKind WitchEntity::getModelKind() const
{
    return gameplay::MobModelKind::Witch;
}
core::ResourceLocation WitchEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/witch");
}
}
