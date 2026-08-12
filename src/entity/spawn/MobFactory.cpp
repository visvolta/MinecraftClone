#include "entity/spawn/MobFactory.h"

#include "entity/mobs/CreeperEntity.h"
#include "entity/mobs/PassiveMobs.h"
#include "entity/mobs/SkeletonEntity.h"
#include "entity/mobs/SlimeEntity.h"
#include "entity/mobs/SpiderEntity.h"
#include "entity/mobs/TameableMobs.h"
#include "entity/mobs/WitchEntity.h"
#include "entity/mobs/ZombieEntity.h"

namespace mc::entity
{
std::unique_ptr<Mob> createMob(const core::ResourceLocation& type, World& world)
{
    const std::string& name = type.path();
    if (name == "zombie") return std::make_unique<ZombieEntity>(world);
    if (name == "husk") return std::make_unique<HuskEntity>(world);
    if (name == "zombie_villager") return std::make_unique<ZombieVillagerEntity>(world);
    if (name == "skeleton") return std::make_unique<SkeletonEntity>(world);
    if (name == "stray") return std::make_unique<StrayEntity>(world);
    if (name == "creeper") return std::make_unique<CreeperEntity>(world);
    if (name == "spider") return std::make_unique<SpiderEntity>(world);
    if (name == "cave_spider") return std::make_unique<CaveSpiderEntity>(world);
    if (name == "slime") return std::make_unique<SlimeEntity>(world);
    if (name == "witch") return std::make_unique<WitchEntity>(world);
    if (name == "cow") return std::make_unique<CowEntity>(world);
    if (name == "mushroom_cow") return std::make_unique<MooshroomEntity>(world);
    if (name == "pig") return std::make_unique<PigEntity>(world);
    if (name == "sheep") return std::make_unique<SheepEntity>(world);
    if (name == "chicken") return std::make_unique<ChickenEntity>(world);
    if (name == "rabbit") return std::make_unique<RabbitEntity>(world);
    if (name == "wolf") return std::make_unique<WolfEntity>(world);
    if (name == "ocelot") return std::make_unique<OcelotEntity>(world);
    if (name == "parrot") return std::make_unique<ParrotEntity>(world);
    if (name == "horse") return std::make_unique<HorseEntity>(world);
    if (name == "donkey") return std::make_unique<DonkeyEntity>(world);
    if (name == "mule") return std::make_unique<MuleEntity>(world);
    if (name == "skeleton_horse") return std::make_unique<SkeletonHorseEntity>(world);
    if (name == "zombie_horse") return std::make_unique<ZombieHorseEntity>(world);
    if (name == "llama") return std::make_unique<LlamaEntity>(world);
    if (name == "bat") return std::make_unique<BatEntity>(world);
    if (name == "squid") return std::make_unique<SquidEntity>(world);
    if (name == "polar_bear") return std::make_unique<PolarBearEntity>(world);
    if (name == "iron_golem") return std::make_unique<IronGolemEntity>(world);
    if (name == "snowman") return std::make_unique<SnowGolemEntity>(world);
    return nullptr;
}
}
