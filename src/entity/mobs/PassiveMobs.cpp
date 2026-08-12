#include "entity/mobs/PassiveMobs.h"

#include "World.h"
#include "entity/Math.h"
#include "entity/PlayerEntity.h"
#include "entity/ai/VanillaGoals.h"
#include "entity/projectile/ArrowEntity.h"

#include <array>
#include <cmath>

namespace mc::entity
{
namespace
{
void addCommonAnimalTasks(AnimalEntity& animal, double panic, double mate,
                          double tempt, double parent, double wander,
                          std::vector<ItemType> foods)
{
    animal.goalSelector().add(0, std::make_unique<ai::SwimGoal>(animal));
    animal.goalSelector().add(1, std::make_unique<ai::PanicGoal>(animal, panic));
    animal.goalSelector().add(2, std::make_unique<ai::MateGoal>(animal, mate));
    animal.goalSelector().add(
        3, std::make_unique<ai::TemptGoal>(animal, tempt, foods, false));
    animal.goalSelector().add(4, std::make_unique<ai::FollowParentGoal>(animal, parent));
    animal.goalSelector().add(5, std::make_unique<ai::WanderAvoidWaterGoal>(animal, wander));
    animal.goalSelector().add(6, std::make_unique<ai::WatchClosestGoal>(animal, 6.0f));
    animal.goalSelector().add(7, std::make_unique<ai::LookIdleGoal>(animal));
}
}

CowEntity::CowEntity(World& world) : AnimalEntity(world)
{
    setSize(0.9f, 1.4f);
}

void CowEntity::initEntityAI()
{
    addCommonAnimalTasks(*this, 2.0, 1.0, 1.25, 1.25, 1.0, {ItemType::WheatItem});
}

void CowEntity::applyEntityAttributes()
{
    AnimalEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(10.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.20000000298023224);
}

std::unique_ptr<AgeableEntity> CowEntity::createChild(AnimalEntity&)
{
    return std::make_unique<CowEntity>(*world_);
}

core::ResourceLocation CowEntity::getType() const
{
    return core::ResourceLocation("minecraft:cow");
}
core::ResourceLocation CowEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/cow");
}
gameplay::MobModelKind CowEntity::getModelKind() const
{
    return gameplay::MobModelKind::Cow;
}
core::ResourceLocation CowEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/cow/cow");
}

MooshroomEntity::MooshroomEntity(World& world) : CowEntity(world) {}

std::unique_ptr<AgeableEntity> MooshroomEntity::createChild(AnimalEntity&)
{
    return std::make_unique<MooshroomEntity>(*world_);
}

bool MooshroomEntity::getCanSpawnHere()
{
    const int x = floorInt(posX);
    const int y = floorInt(boundingBox_.minY);
    const int z = floorInt(posZ);
    return world_->getBlock(x, y - 1, z) == BlockType::Mycelium &&
           world_->getSkyLightLevel(x, y, z) > 8 && isNotColliding();
}

core::ResourceLocation MooshroomEntity::getType() const
{
    return core::ResourceLocation("minecraft:mushroom_cow");
}
core::ResourceLocation MooshroomEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/mushroom_cow");
}
core::ResourceLocation MooshroomEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/cow/mooshroom");
}

PigEntity::PigEntity(World& world) : AnimalEntity(world)
{
    setSize(0.9f, 0.9f);
}

void PigEntity::initEntityAI()
{
    addCommonAnimalTasks(
        *this, 1.25, 1.0, 1.2, 1.1, 1.0,
        {ItemType::Carrot, ItemType::Potato, ItemType::BeetrootItem});
}

void PigEntity::applyEntityAttributes()
{
    AnimalEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(10.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.25);
}

bool PigEntity::isBreedingItem(ItemType item) const
{
    return item == ItemType::Carrot || item == ItemType::Potato ||
           item == ItemType::BeetrootItem;
}

std::unique_ptr<AgeableEntity> PigEntity::createChild(AnimalEntity&)
{
    return std::make_unique<PigEntity>(*world_);
}

core::ResourceLocation PigEntity::getType() const
{
    return core::ResourceLocation("minecraft:pig");
}
core::ResourceLocation PigEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/pig");
}
gameplay::MobModelKind PigEntity::getModelKind() const
{
    return gameplay::MobModelKind::Pig;
}
core::ResourceLocation PigEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/pig/pig");
}

ChickenEntity::ChickenEntity(World& world) : AnimalEntity(world)
{
    setSize(0.4f, 0.7f);
    timeUntilNextEgg_ = rand_.nextInt(6000) + 6000;
}

void ChickenEntity::initEntityAI()
{
    addCommonAnimalTasks(
        *this, 1.4, 1.0, 1.0, 1.1, 1.0,
        {ItemType::Seeds, ItemType::MelonSeeds, ItemType::PumpkinSeeds,
         ItemType::BeetrootSeeds});
}

void ChickenEntity::applyEntityAttributes()
{
    AnimalEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(4.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.25);
}

bool ChickenEntity::isBreedingItem(ItemType item) const
{
    return item == ItemType::Seeds || item == ItemType::MelonSeeds ||
           item == ItemType::PumpkinSeeds || item == ItemType::BeetrootSeeds;
}

void ChickenEntity::onLivingUpdate()
{
    AnimalEntity::onLivingUpdate();
    if (!isChild() && --timeUntilNextEgg_ <= 0)
        timeUntilNextEgg_ = rand_.nextInt(6000) + 6000;
    if (!onGround && motionY < 0.0)
        motionY *= 0.6;
}

std::unique_ptr<AgeableEntity> ChickenEntity::createChild(AnimalEntity&)
{
    return std::make_unique<ChickenEntity>(*world_);
}

core::ResourceLocation ChickenEntity::getType() const
{
    return core::ResourceLocation("minecraft:chicken");
}
core::ResourceLocation ChickenEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/chicken");
}
gameplay::MobModelKind ChickenEntity::getModelKind() const
{
    return gameplay::MobModelKind::Chicken;
}
core::ResourceLocation ChickenEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/chicken");
}

RabbitEntity::RabbitEntity(World& world) : AnimalEntity(world)
{
    setSize(0.4f, 0.5f);
}

void RabbitEntity::initEntityAI()
{
    tasks_.add(1, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(1, std::make_unique<ai::PanicGoal>(*this, 2.2));
    tasks_.add(2, std::make_unique<ai::MateGoal>(*this, 0.8));
    tasks_.add(3, std::make_unique<ai::TemptGoal>(
        *this, 1.0,
        std::vector<ItemType>{ItemType::Carrot, ItemType::GoldenCarrot},
        false));
    tasks_.add(4, std::make_unique<ai::AvoidEntityGoal>(
        *this, 8.0f, 2.2, 2.2,
        [](LivingEntity& e) { return e.isPlayer(); }));
    tasks_.add(6, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 0.6));
    tasks_.add(11, std::make_unique<ai::WatchClosestGoal>(*this, 10.0f));
}

void RabbitEntity::applyEntityAttributes()
{
    AnimalEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(3.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.3);
}

bool RabbitEntity::isBreedingItem(ItemType item) const
{
    return item == ItemType::Carrot || item == ItemType::GoldenCarrot;
}

void RabbitEntity::onInitialSpawn()
{
    AnimalEntity::onInitialSpawn();
    const BiomeId biome = world_->getBiomeAt(floorInt(posX), floorInt(posZ));
    const auto* def = BiomeRegistry::active().find(biome);
    const int roll = rand_.nextInt(100);
    if (def && def->snowy)
        variant_ = roll < 80 ? 1 : 3;
    else if (def && def->name.path().find("desert") != std::string::npos)
        variant_ = 4;
    else
        variant_ = roll < 50 ? 0 : roll < 90 ? 5 : 2;
}

std::unique_ptr<AgeableEntity> RabbitEntity::createChild(AnimalEntity& mate)
{
    auto child = std::make_unique<RabbitEntity>(*world_);
    child->variant_ = rand_.nextBoolean()
        ? variant_
        : static_cast<RabbitEntity&>(mate).variant_;
    return child;
}

core::ResourceLocation RabbitEntity::getType() const
{
    return core::ResourceLocation("minecraft:rabbit");
}
core::ResourceLocation RabbitEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/rabbit");
}
gameplay::MobModelKind RabbitEntity::getModelKind() const
{
    return gameplay::MobModelKind::Rabbit;
}
core::ResourceLocation RabbitEntity::getTexture() const
{
    static const char* names[] = {
        "brown", "white", "black", "white_splotched", "gold", "salt"};
    const int index = std::clamp(variant_, 0, 5);
    return core::ResourceLocation(
        "minecraft", std::string("entity/rabbit/") + names[index]);
}

int SheepEntity::getRandomSheepColor(JavaRandom& random)
{
    const int roll = random.nextInt(100);
    if (roll < 5) return 15;
    if (roll < 10) return 7;
    if (roll < 15) return 8;
    if (roll < 18) return 12;
    return random.nextInt(500) == 0 ? 6 : 0;
}

SheepEntity::SheepEntity(World& world) : AnimalEntity(world)
{
    setSize(0.9f, 1.3f);
}

void SheepEntity::initEntityAI()
{
    tasks_.add(0, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(1, std::make_unique<ai::PanicGoal>(*this, 1.25));
    tasks_.add(2, std::make_unique<ai::MateGoal>(*this, 1.0));
    tasks_.add(3, std::make_unique<ai::TemptGoal>(
        *this, 1.1, std::vector<ItemType>{ItemType::WheatItem}, false));
    tasks_.add(4, std::make_unique<ai::FollowParentGoal>(*this, 1.1));
    tasks_.add(5, std::make_unique<ai::EatGrassGoal>(*this));
    tasks_.add(6, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 1.0));
    tasks_.add(7, std::make_unique<ai::WatchClosestGoal>(*this, 6.0f));
    tasks_.add(8, std::make_unique<ai::LookIdleGoal>(*this));
}

void SheepEntity::applyEntityAttributes()
{
    AnimalEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(8.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)
        .setBaseValue(0.23000000417232513);
}

void SheepEntity::eatGrassBonus()
{
    sheared_ = false;
    if (isChild())
        addGrowth(60);
}

bool SheepEntity::processInteract(PlayerEntity& player, ItemStack& stack)
{
    if (stack.item == ItemType::Shears && !sheared_ && !isChild())
    {
        sheared_ = true;
        stack.damageItem(1);
        const int count = 1 + rand_.nextInt(3);
        constexpr std::array<BlockType, 16> wool{{
            BlockType::WhiteWool, BlockType::OrangeWool, BlockType::MagentaWool,
            BlockType::LightBlueWool, BlockType::YellowWool, BlockType::LimeWool,
            BlockType::PinkWool, BlockType::GrayWool, BlockType::LightGrayWool,
            BlockType::CyanWool, BlockType::PurpleWool, BlockType::BlueWool,
            BlockType::BrownWool, BlockType::GreenWool, BlockType::RedWool,
            BlockType::BlackWool
        }};
        for (int i = 0; i < count; ++i)
            world_->spawnItemStack(
                ItemStack(itemFromBlock(wool[static_cast<std::size_t>(
                    std::clamp(fleeceColor_, 0, 15))]), 1),
                posX, posY + 1.0, posZ);
        return true;
    }
    return AnimalEntity::processInteract(player, stack);
}

void SheepEntity::onInitialSpawn()
{
    AnimalEntity::onInitialSpawn();
    fleeceColor_ = getRandomSheepColor(rand_);
}

std::unique_ptr<AgeableEntity> SheepEntity::createChild(AnimalEntity& mate)
{
    auto child = std::make_unique<SheepEntity>(*world_);
    const int other = static_cast<SheepEntity&>(mate).fleeceColor_;
    child->fleeceColor_ = rand_.nextBoolean() ? fleeceColor_ : other;
    return child;
}

core::ResourceLocation SheepEntity::getType() const
{
    return core::ResourceLocation("minecraft:sheep");
}
core::ResourceLocation SheepEntity::getLootTable() const
{
    return sheared_
        ? core::ResourceLocation("minecraft:entities/sheep")
        : core::ResourceLocation("minecraft:entities/sheep");
}
gameplay::MobModelKind SheepEntity::getModelKind() const
{
    return gameplay::MobModelKind::Sheep;
}
core::ResourceLocation SheepEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/sheep/sheep");
}
core::ResourceLocation SheepEntity::getOverlayTexture() const
{
    return core::ResourceLocation("minecraft:entity/sheep/sheep_fur");
}
glm::vec3 SheepEntity::getOverlayColour() const
{
    constexpr std::array<std::uint32_t, 16> colours{{
        16383998U, 16351261U, 13061821U, 3847130U, 16701501U, 8439583U,
        15961002U, 4673362U, 10329495U, 1481884U, 8991416U, 3949738U,
        8606770U, 6192150U, 11546150U, 1908001U
    }};
    const int metadata = std::clamp(fleeceColor_, 0, 15);
    if (metadata == 0)
        return {0.9019608f, 0.9019608f, 0.9019608f};
    const std::uint32_t colour = colours[static_cast<std::size_t>(metadata)];
    return {
        static_cast<float>((colour >> 16U) & 255U) / 255.0f * 0.75f,
        static_cast<float>((colour >> 8U) & 255U) / 255.0f * 0.75f,
        static_cast<float>(colour & 255U) / 255.0f * 0.75f
    };
}

BatEntity::BatEntity(World& world) : Mob(world)
{
    setSize(0.5f, 0.9f);
    experienceValue_ = 0;
}

void BatEntity::applyEntityAttributes()
{
    Mob::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(6.0);
}

void BatEntity::onUpdate()
{
    Mob::onUpdate();
    if (hanging_)
    {
        motionX = motionY = motionZ = 0.0;
        posY = std::floor(posY) + 1.0 - static_cast<double>(height_);
    }
    else
    {
        motionY *= 0.6000000238418579;
    }
}

bool BatEntity::getCanSpawnHere()
{
    const int y = floorInt(posY);
    if (y >= 63)
        return false;
    return world_->getBlockLightLevel(floorInt(posX), y, floorInt(posZ)) <=
           rand_.nextInt(7) && isNotColliding();
}

core::ResourceLocation BatEntity::getType() const
{
    return core::ResourceLocation("minecraft:bat");
}
core::ResourceLocation BatEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/bat");
}
gameplay::MobModelKind BatEntity::getModelKind() const
{
    return gameplay::MobModelKind::Bat;
}
core::ResourceLocation BatEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/bat");
}

SquidEntity::SquidEntity(World& world) : Mob(world)
{
    setSize(0.8f, 0.8f);
    canBreatheUnderwater_ = true;
}

void SquidEntity::applyEntityAttributes()
{
    Mob::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(10.0);
}

void SquidEntity::onLivingUpdate()
{
    Mob::onLivingUpdate();
    if (!isInWater())
        motionY -= 0.08;
}

bool SquidEntity::getCanSpawnHere()
{
    const int y = floorInt(posY);
    return y > 45 && y < 63 &&
           world_->getBlock(floorInt(posX), y, floorInt(posZ)) == BlockType::Water;
}

core::ResourceLocation SquidEntity::getType() const
{
    return core::ResourceLocation("minecraft:squid");
}
core::ResourceLocation SquidEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/squid");
}
gameplay::MobModelKind SquidEntity::getModelKind() const
{
    return gameplay::MobModelKind::Squid;
}
core::ResourceLocation SquidEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/squid");
}

IronGolemEntity::IronGolemEntity(World& world) : Creature(world)
{
    setSize(1.4f, 2.7f);
    experienceValue_ = 0;
    persistenceRequired_ = true;
}

void IronGolemEntity::initEntityAI()
{
    tasks_.add(1, std::make_unique<ai::AttackMeleeGoal>(*this, 1.0, true));
    tasks_.add(2, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 0.6, 1.0f / 120.0f));
    tasks_.add(3, std::make_unique<ai::WatchClosestGoal>(*this, 6.0f));
    tasks_.add(4, std::make_unique<ai::LookIdleGoal>(*this));
    targetTasks_.add(2, std::make_unique<ai::HurtByTargetGoal>(*this, false));
}

void IronGolemEntity::applyEntityAttributes()
{
    Creature::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(100.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.25);
    attributes_.registerAttribute(SharedMonsterAttributes::ATTACK_DAMAGE).setBaseValue(15.0);
}

bool IronGolemEntity::attackEntityAsMob(Entity& target)
{
    const bool hit = Creature::attackEntityAsMob(target);
    if (hit)
        target.motionY += 0.4000000059604645;
    return hit;
}

core::ResourceLocation IronGolemEntity::getType() const
{
    return core::ResourceLocation("minecraft:iron_golem");
}
core::ResourceLocation IronGolemEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/iron_golem");
}
gameplay::MobModelKind IronGolemEntity::getModelKind() const
{
    return gameplay::MobModelKind::IronGolem;
}
core::ResourceLocation IronGolemEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/iron_golem");
}

SnowGolemEntity::SnowGolemEntity(World& world) : Creature(world)
{
    setSize(0.7f, 1.9f);
    persistenceRequired_ = true;
}

void SnowGolemEntity::initEntityAI()
{
    tasks_.add(1, std::make_unique<ai::AttackRangedBowGoal>(*this, 1.25, 20, 10.0f));
    tasks_.add(2, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 1.0, 1.0f / 120.0f));
    tasks_.add(3, std::make_unique<ai::WatchClosestGoal>(*this, 6.0f));
    tasks_.add(4, std::make_unique<ai::LookIdleGoal>(*this));
    targetTasks_.add(1, std::make_unique<ai::NearestAttackableTargetGoal>(*this, false));
}

void SnowGolemEntity::applyEntityAttributes()
{
    Creature::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(4.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.20000000298023224);
}

void SnowGolemEntity::onLivingUpdate()
{
    Creature::onLivingUpdate();
    if (world_->getTemperatureAt(floorInt(posX), floorInt(posZ)) > 1.0f)
        attackEntityFrom(DamageSource::ON_FIRE, 1.0f);
}

bool SnowGolemEntity::attackEntityAsMob(Entity& target)
{
    auto snowball = std::make_unique<SnowballEntity>(getWorld(), this);
    const double dx = target.posX - posX;
    const double dy = target.posY + target.getHeight() * 0.333 - snowball->posY;
    const double dz = target.posZ - posZ;
    const float dist = static_cast<float>(std::sqrt(dx * dx + dz * dz));
    snowball->shoot(dx, dy + dist * 0.2, dz, 1.6f, 12.0f);
    getWorld().spawnEntity(std::move(snowball));
    return true;
}

core::ResourceLocation SnowGolemEntity::getType() const
{
    return core::ResourceLocation("minecraft:snowman");
}
core::ResourceLocation SnowGolemEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/snowman");
}
gameplay::MobModelKind SnowGolemEntity::getModelKind() const
{
    return gameplay::MobModelKind::SnowGolem;
}
core::ResourceLocation SnowGolemEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/snowman");
}

PolarBearEntity::PolarBearEntity(World& world) : AnimalEntity(world)
{
    setSize(1.3f, 1.4f);
}

void PolarBearEntity::initEntityAI()
{
    tasks_.add(0, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(1, std::make_unique<ai::AttackMeleeGoal>(*this, 1.25, false));
    tasks_.add(1, std::make_unique<ai::PanicGoal>(*this, 2.0));
    tasks_.add(4, std::make_unique<ai::FollowParentGoal>(*this, 1.25));
    tasks_.add(5, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 1.0, 1.0f / 120.0f));
    tasks_.add(6, std::make_unique<ai::WatchClosestGoal>(*this, 6.0f));
    tasks_.add(7, std::make_unique<ai::LookIdleGoal>(*this));
    targetTasks_.add(1, std::make_unique<ai::HurtByTargetGoal>(*this, false));
}

void PolarBearEntity::applyEntityAttributes()
{
    AnimalEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(30.0);
    getEntityAttribute(SharedMonsterAttributes::FOLLOW_RANGE).setBaseValue(20.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.25);
    attributes_.registerAttribute(SharedMonsterAttributes::ATTACK_DAMAGE).setBaseValue(6.0);
}

std::unique_ptr<AgeableEntity> PolarBearEntity::createChild(AnimalEntity&)
{
    return std::make_unique<PolarBearEntity>(*world_);
}

core::ResourceLocation PolarBearEntity::getType() const
{
    return core::ResourceLocation("minecraft:polar_bear");
}
core::ResourceLocation PolarBearEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/polar_bear");
}
gameplay::MobModelKind PolarBearEntity::getModelKind() const
{
    return gameplay::MobModelKind::PolarBear;
}
core::ResourceLocation PolarBearEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/bear/polarbear");
}
}
